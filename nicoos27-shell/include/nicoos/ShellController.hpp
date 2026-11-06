#pragma once

#include <QObject>
#include <QString>

namespace nicoos {

// Hub dell'intero processo, esposto a ogni superficie QML come proprietà di
// contesto "shell". Il QML chiede le cose per nome ("launcher", "power", ...)
// e ShellApplication — che possiede gli oggetti veri — le esegue.
class ShellController : public QObject {
    Q_OBJECT

public:
    static ShellController& instance();

    // Attiva/disattiva un popup per nome: launcher, controlcenter, workspaces,
    // power, wallpapers, clipboard, calendar.
    Q_INVOKABLE void toggle(const QString& name) { emit toggleRequested(name); }
    // Esegue un'azione con nome, ad esempio "screenshot.region",
    // "dnd.toggle".
    Q_INVOKABLE void run(const QString& action) { emit actionRequested(action); }
    Q_INVOKABLE void hideAll() { emit hideAllRequested(); }

signals:
    void toggleRequested(const QString& name);
    void actionRequested(const QString& action);
    void hideAllRequested();

private:
    ShellController() = default;
};

} // namespace nicoos
