#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace nicoos {

// Espone al QML un piccolo insieme fisso di azioni di sessione e alimentazione
// come verbi con nome ("lock", "suspend", "logout", "reboot", "poweroff")
// invece di lasciare che il QML passi stringhe di comandi di shell
// arbitrari.
//
// Blocco: hyprlock viene avviato con la configurazione generata dalla shell
// (~/.config/nicoos/hyprlock.conf, vedi hyprlockConfigPath()) invece di quella
// che capita di avere in ~/.config/hypr. Se un bloccante muore subito dopo
// l'avvio, si prova il successivo (hyprlock con la sua configurazione
// predefinita, poi swaylock, gtklock). Se muore PIÙ TARDI, cioè va in crash
// mentre la sessione è bloccata (i compositori tengono lo schermo bloccato e
// mostrano un errore), viene riavviato automaticamente così lo schermo può
// comunque essere sbloccato. L'output dei bloccanti finisce in
// ~/.cache/nicoos/lock.log.
//
// Logout: prova uwsm, poi il dispatcher `exit` di Hyprland, quindi ripiega su
// hyprctl, loginctl e infine pkill, e segnala un fallimento se nessuno ha
// funzionato.
class PowerActions : public QObject {
    Q_OBJECT

public:
    explicit PowerActions(QObject* parent = nullptr);

    Q_INVOKABLE void trigger(const QString& action);
    bool hasLocker() const;

    // Scrive (o aggiorna) la configurazione hyprlock generata e ne restituisce
    // il percorso; rispetta [lock] hyprlock_config. Stringa vuota in caso di
    // errore.
    static QString hyprlockConfigPath();

signals:
    void actionFailed(const QString& action, const QString& reason);

private:
    void lock();
    void startLocker(int index, QStringList failures);
    void lockUnavailable(const QStringList& failures);
    void suspend();
    void logout();
    void scheduleLogoutFallbacks();
    void reboot();
    void poweroff();

    // Timestamp (ms dall'epoca) dei recenti riavvii del bloccante dopo un
    // crash.
    QVector<qint64> m_respawns;
};

} // namespace nicoos
