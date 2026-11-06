#include "nicoos/NetworkManagerClient.hpp"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDebug>

namespace nicoos {

namespace {
constexpr auto NM_SERVICE = "org.freedesktop.NetworkManager";
constexpr auto NM_PATH = "/org/freedesktop/NetworkManager";
constexpr auto NM_IFACE = "org.freedesktop.NetworkManager";
constexpr auto NM_ACTIVE_IFACE = "org.freedesktop.NetworkManager.Connection.Active";
constexpr auto NM_DEVICE_IFACE = "org.freedesktop.NetworkManager.Device";
constexpr auto NM_WIRELESS_IFACE = "org.freedesktop.NetworkManager.Device.Wireless";
constexpr auto NM_AP_IFACE = "org.freedesktop.NetworkManager.AccessPoint";
constexpr auto PROPS_IFACE = "org.freedesktop.DBus.Properties";

// NM_STATE_CONNECTED_LOCAL = 50, CONNECTED_SITE = 60, CONNECTED_GLOBAL = 70
constexpr uint NM_STATE_CONNECTED_LOCAL = 50;
// NM_DEVICE_TYPE_WIFI
constexpr uint NM_DEVICE_TYPE_WIFI = 2;
} // namespace

NetworkManagerClient::NetworkManagerClient(QObject* parent) : QObject(parent) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(120);
    connect(&m_debounce, &QTimer::timeout, this, &NetworkManagerClient::refreshState);

    auto bus = QDBusConnection::systemBus();
    if (!bus.isConnected()) {
        qWarning() << "[NetworkManagerClient] bus di sistema non raggiungibile;"
                      " lo stato della rete sarà mostrato come non connesso.";
        return;
    }

    // Considera NetworkManager presente solo se qualcuno possiede davvero il
    // nome; altrimenti resta in silenzio e mantieni i valori predefiniti di
    // "non connesso".
    const auto owner = bus.interface()->isServiceRegistered(NM_SERVICE);
    m_available = owner.isValid() && owner.value();
    if (!m_available) {
        qWarning() << "[NetworkManagerClient] NetworkManager non è in esecuzione;"
                      " lo stato della rete sarà mostrato come non connesso.";
        return;
    }

    bus.connect(NM_SERVICE, NM_PATH, NM_IFACE, "StateChanged",
                this, SLOT(scheduleRefresh()));
    bus.connect(NM_SERVICE, NM_PATH, PROPS_IFACE, "PropertiesChanged",
                this, SLOT(scheduleRefresh()));

    refreshState();
}

QVariant NetworkManagerClient::getProperty(const QString& path, const QString& iface,
                                           const QString& name) const {
    QDBusMessage msg = QDBusMessage::createMethodCall(NM_SERVICE, path, PROPS_IFACE, "Get");
    msg << iface << name;
    const QDBusMessage reply = QDBusConnection::systemBus().call(msg, QDBus::Block, 1500);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) return {};
    return reply.arguments().at(0).value<QDBusVariant>().variant();
}

void NetworkManagerClient::scheduleRefresh() {
    m_debounce.start();
}

void NetworkManagerClient::resetState() {
    m_kind = NetworkKind::None;
    m_connected = false;
    m_ssid.clear();
    m_signalStrength = 0;
    watchAccessPoint(QString());
}

// Si iscrive a PropertiesChanged del punto di accesso a cui siamo associati,
// così la lettura dell'intensità del segnale segue la radio reale. Si osserva
// un solo AP alla volta.
void NetworkManagerClient::watchAccessPoint(const QString& apPath) {
    if (apPath == m_watchedAp) return;
    auto bus = QDBusConnection::systemBus();
    if (!m_watchedAp.isEmpty()) {
        bus.disconnect(NM_SERVICE, m_watchedAp, PROPS_IFACE, "PropertiesChanged",
                       this, SLOT(scheduleRefresh()));
    }
    m_watchedAp = apPath;
    if (!m_watchedAp.isEmpty()) {
        bus.connect(NM_SERVICE, m_watchedAp, PROPS_IFACE, "PropertiesChanged",
                    this, SLOT(scheduleRefresh()));
    }
}

void NetworkManagerClient::refreshState() {
    if (!m_available) return;

    // Istantanea di ciò che il QML vede ora, per emettere stateChanged() solo
    // quando qualcosa è cambiato davvero.
    const bool oldConnected = m_connected;
    const QString oldSsid = m_ssid;
    const int oldSignal = m_signalStrength;
    const NetworkKind oldKind = m_kind;
    const bool oldWifiEnabled = m_wifiEnabled;

    const QVariant stateVar = getProperty(NM_PATH, NM_IFACE, "State");
    const uint state = stateVar.isValid() ? stateVar.toUInt() : 0;
    const bool linkUp = state >= NM_STATE_CONNECTED_LOCAL && state <= 70;

    const QVariant wifiVar = getProperty(NM_PATH, NM_IFACE, "WirelessEnabled");
    if (wifiVar.isValid()) m_wifiEnabled = wifiVar.toBool();

    resetState();
    m_connected = linkUp;

    if (linkUp) {
        const QVariant primary = getProperty(NM_PATH, NM_IFACE, "PrimaryConnection");
        const QString activePath = primary.value<QDBusObjectPath>().path();

        if (!activePath.isEmpty() && activePath != "/") {
            m_ssid = getProperty(activePath, NM_ACTIVE_IFACE, "Id").toString();

            const QString connType = getProperty(activePath, NM_ACTIVE_IFACE, "Type").toString();
            if (connType.contains("wireless")) m_kind = NetworkKind::WiFi;
            else if (connType.contains("ethernet")) m_kind = NetworkKind::Ethernet;
            else m_kind = NetworkKind::Unknown;

            if (m_kind == NetworkKind::WiFi) {
                // Lo SpecificObject di una connessione Wi-Fi attiva è l'AP.
                const QString apPath =
                    getProperty(activePath, NM_ACTIVE_IFACE, "SpecificObject")
                        .value<QDBusObjectPath>().path();
                if (!apPath.isEmpty() && apPath != "/") {
                    const QVariant strength = getProperty(apPath, NM_AP_IFACE, "Strength");
                    m_signalStrength = strength.isValid() ? strength.toInt() : 0;
                    watchAccessPoint(apPath);
                }
            } else {
                m_signalStrength = 100;
            }
        } else {
            // Collegato ma senza connessione primaria (ad esempio solo
            // locale): mantieni un'etichetta generica sensata invece di una
            // vecchia.
            m_kind = NetworkKind::Unknown;
        }
    }

    if (oldConnected != m_connected || oldSsid != m_ssid || oldSignal != m_signalStrength ||
        oldKind != m_kind || oldWifiEnabled != m_wifiEnabled) {
        emit stateChanged();
    }
}

QString NetworkManagerClient::kindLabel() const {
    switch (m_kind) {
        case NetworkKind::WiFi:     return "Wi-Fi";
        case NetworkKind::Ethernet: return "Ethernet";
        case NetworkKind::None:     return "Non connesso";
        default:                    return "Sconosciuto";
    }
}

void NetworkManagerClient::requestRescan() {
    if (!m_available) return;
    auto bus = QDBusConnection::systemBus();

    QDBusMessage list = QDBusMessage::createMethodCall(NM_SERVICE, NM_PATH, NM_IFACE, "GetDevices");
    const QDBusReply<QList<QDBusObjectPath>> devices = bus.call(list, QDBus::Block, 1500);
    if (!devices.isValid()) return;

    for (const QDBusObjectPath& dev : devices.value()) {
        const QVariant type = getProperty(dev.path(), NM_DEVICE_IFACE, "DeviceType");
        if (!type.isValid() || type.toUInt() != NM_DEVICE_TYPE_WIFI) continue;

        QDBusMessage scan = QDBusMessage::createMethodCall(NM_SERVICE, dev.path(),
                                                           NM_WIRELESS_IFACE, "RequestScan");
        scan << QVariantMap();  // dizionario di opzioni vuoto (a{sv})
        bus.asyncCall(scan);
    }
}

void NetworkManagerClient::toggleWifi(bool enabled) {
    if (!m_available) return;
    // Impostare una proprietà su una QDBusInterface non è affidabile con
    // NetworkManager; il modo standard è org.freedesktop.DBus.Properties.Set.
    QDBusMessage msg = QDBusMessage::createMethodCall(NM_SERVICE, NM_PATH, PROPS_IFACE, "Set");
    msg << QString(NM_IFACE) << QString("WirelessEnabled")
        << QVariant::fromValue(QDBusVariant(enabled));
    QDBusConnection::systemBus().asyncCall(msg);
}

} // namespace nicoos
