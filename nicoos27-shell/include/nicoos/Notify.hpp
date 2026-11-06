#pragma once

#include <QDBusConnection>
#include <QDBusMessage>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace nicoos {

// Mostra una notifica tramite il nostro servizio D-Bus
// org.freedesktop.Notifications. Senza attesa di risposta, nessuna dipendenza
// da libnotify, e non scarta mai messaggi in silenzio anche se notify-send non
// è installato.
inline void notifyUser(const QString& title, const QString& body = QString(), int timeoutMs = 7000) {
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("Notify"));
    msg.setArguments({
        QStringLiteral("NicoOS"),
        uint(0),
        QString(),
        title,
        body,
        QStringList(),
        QVariantMap(),
        timeoutMs
    });
    // asyncCall: non bloccante, non solleva mai eccezioni (ritorno void).
    QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
}

} // namespace nicoos
