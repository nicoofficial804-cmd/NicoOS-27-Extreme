#pragma once

#include <QObject>
#include <QDBusContext>
#include <QDBusArgument>
#include <QVariantMap>
#include <QVector>
#include <QString>
#include <memory>

namespace nicoos {

struct NotificationEntry {
    uint id{};
    QString appName;
    QString summary;
    QString body;
    QString iconName;
    int timeoutMs{5000};
};

class NotificationWindow;

// Si registra sul bus di sessione come org.freedesktop.Notifications, così le
// applicazioni Linux standard (browser, client di chat, unità systemd tramite
// notify-send, ecc.) possono mostrare notifiche con il normale meccanismo del
// desktop invece che con qualcosa di specifico di NicoOS.
class NotificationDaemon : public QObject, protected QDBusContext {
    Q_OBJECT

public:
    explicit NotificationDaemon(QObject* parent = nullptr);
    // Dichiarato qui ma definito nel .cpp: con la sola dichiarazione
    // anticipata di NotificationWindow in questo header, un distruttore
    // generato implicitamente (inline) proverebbe a istanziare il deleter di
    // std::unique_ptr<NotificationWindow> su un tipo incompleto in ogni punto
    // di inclusione. Definirlo fuori linea in NotificationDaemon.cpp, dove è
    // incluso NotificationWindow.hpp, rinvia quell'istanza all'unico punto in
    // cui il tipo è completo.
    ~NotificationDaemon() override;

    bool registerService();

    // Non disturbare: se vero i popup vengono scartati (le notifiche con
    // urgenza critica passano comunque).
    void setSuppressPopups(bool suppress) { m_suppress = suppress; }

public slots:
    // Metodi esposti su D-Bus con la firma richiesta dalle specifiche
    // Notifications di freedesktop.
    uint Notify(const QString& appName, uint replacesId, const QString& appIcon,
                const QString& summary, const QString& body,
                const QStringList& actions, const QVariantMap& hints, int timeout);

    void CloseNotification(uint id);
    QStringList GetCapabilities();
    QString GetServerInformation_name();

signals:
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString& actionKey);

    // Segnale Qt interno (non esposto su D-Bus) usato per pilotare la finestra
    // e la coda dei popup delle notifiche.
    void notificationReceived(const NotificationEntry& entry);

private:
    uint m_nextId{1};
    bool m_suppress{false};
    std::unique_ptr<NotificationWindow> m_window;
};

} // namespace nicoos

Q_DECLARE_METATYPE(nicoos::NotificationEntry)
