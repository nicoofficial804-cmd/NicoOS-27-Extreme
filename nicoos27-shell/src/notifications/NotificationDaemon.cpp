#include "nicoos/NotificationDaemon.hpp"
#include "nicoos/NotificationWindow.hpp"

#include <QDBusConnection>
#include <QDebug>

namespace nicoos {

NotificationDaemon::NotificationDaemon(QObject* parent) : QObject(parent) {
    m_window = std::make_unique<NotificationWindow>();
    connect(this, &NotificationDaemon::notificationReceived,
            m_window.get(), &NotificationWindow::enqueue);
}

NotificationDaemon::~NotificationDaemon() = default;

bool NotificationDaemon::registerService() {
    auto bus = QDBusConnection::sessionBus();

    if (!bus.registerService("org.freedesktop.Notifications")) {
        qWarning() << "[NotificationDaemon] impossibile registrare org.freedesktop.Notifications"
                      " — probabilmente c'e' gia' un altro demone di notifiche (es. mako, dunst)"
                      " in esecuzione. Le notifiche NicoOS verranno disattivate per evitare conflitti.";
        return false;
    }

    if (!bus.registerObject("/org/freedesktop/Notifications", this,
                             QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals)) {
        qWarning() << "[NotificationDaemon] registrazione dell'oggetto D-Bus non riuscita.";
        return false;
    }

    return true;
}

uint NotificationDaemon::Notify(const QString& appName, uint replacesId, const QString& appIcon,
                                 const QString& summary, const QString& body,
                                 const QStringList& /*actions*/, const QVariantMap& hints,
                                 int timeout) {
    uint id = (replacesId != 0) ? replacesId : m_nextId++;

    NotificationEntry entry;
    entry.id = id;
    entry.appName = appName;
    entry.summary = summary;
    entry.body = body;
    entry.iconName = appIcon;
    entry.timeoutMs = (timeout > 0) ? timeout : 5000;

    // hint di urgenza: 0 bassa, 1 normale, 2 critica
    const int urgency = hints.value("urgency", 1).toInt();
    if (m_suppress && urgency < 2) {
        return id;  // Non disturbare: accettata ma non mostrata
    }

    emit notificationReceived(entry);
    return id;
}

void NotificationDaemon::CloseNotification(uint id) {
    m_window->dismiss(id);
    emit NotificationClosed(id, 3 /* CLOSED_BY_CALLER */);
}

QStringList NotificationDaemon::GetCapabilities() {
    return {"body", "icon-static", "persistence"};
}

QString NotificationDaemon::GetServerInformation_name() {
    return "Notifiche NicoOS";
}

} // namespace nicoos
