#include "nicoos/BluetoothClient.hpp"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QDebug>

namespace nicoos {

BluetoothClient::BluetoothClient(QObject* parent) : QObject(parent) {
    // hci0 è di gran lunga il percorso predefinito più comune per
    // l'adattatore; una build di produzione enumererebbe gli oggetti di
    // org.bluez.Manager1, ma questo copre in modo pulito il caso standard con
    // un solo adattatore.
    m_adapterPath = "/org/bluez/hci0";

    m_props = std::make_unique<QDBusInterface>(
        "org.bluez", m_adapterPath, "org.freedesktop.DBus.Properties",
        QDBusConnection::systemBus());

    m_available = QDBusConnection::systemBus().interface()->isServiceRegistered("org.bluez");

    if (m_available) {
        QDBusConnection::systemBus().connect(
            "org.bluez", m_adapterPath, "org.freedesktop.DBus.Properties",
            "PropertiesChanged", this, SLOT(refresh()));
        refresh();
    } else {
        qWarning() << "[BluetoothClient] org.bluez non registrato; attivazione Bluetooth disabilitata.";
    }
}

void BluetoothClient::refresh() {
    if (!m_available) return;
    QDBusReply<QDBusVariant> reply = m_props->call("Get", "org.bluez.Adapter1", "Powered");
    if (reply.isValid()) {
        m_powered = reply.value().variant().toBool();
        emit poweredChanged();
    }
}

void BluetoothClient::setPowered(bool on) {
    if (!m_available) return;
    m_props->call("Set", "org.bluez.Adapter1", "Powered", QVariant::fromValue(QDBusVariant(on)));
    m_powered = on;
    emit poweredChanged();
}

} // namespace nicoos
