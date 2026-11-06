#include "nicoos/ConfigManager.hpp"
#include "nicoos/ProcessUtil.hpp"
#include "nicoos/ShellApplication.hpp"

#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QQuickStyle>
#include <QDebug>

#if defined(NICOOS_HAVE_LAYER_SHELL)
#include <LayerShellQt/Shell>
#endif

int main(int argc, char* argv[]) {
#if defined(NICOOS_HAVE_LAYER_SHELL)
    // Deve girare prima che esista la QGuiApplication: seleziona
    // l'integrazione Wayland layer-shell per ogni finestra che creiamo.
    LayerShellQt::Shell::useLayerShell();
#endif

    // In una macchina virtuale la GPU emulata di solito non riesce a passare a
    // Qt buffer validi (Hyprland ci disconnette con
    // "wl_surface.attach: invalid arguments"). Lì si usa il renderer software.
    // Si può forzare con NICOOS_FORCE_GPU=1, oppure impostando QT_QUICK_BACKEND
    // direttamente.
    if (nicoos::runningInVirtualMachine() && !qEnvironmentVariableIsSet("QT_QUICK_BACKEND") &&
        qEnvironmentVariable("NICOOS_FORCE_GPU") != "1") {
        qputenv("QT_QUICK_BACKEND", "software");
        qInfo() << "[main] macchina virtuale rilevata: uso il renderer software di Qt Quick"
                   " (NICOOS_FORCE_GPU=1 per disattivarlo)";
    }

    // Forza lo stile "Basic" di QtQuick Controls, che non dipende dal tema
    // nativo della piattaforma, così è il nostro QML a definire l'aspetto.
    QQuickStyle::setStyle("Basic");

    QGuiApplication app(argc, argv);
    app.setApplicationName("nicoos-shell");
    app.setOrganizationName("NicoOS");
    // Nascondere un popup (launcher, centro di controllo) non deve terminare
    // la sessione: il pannello e il demone delle notifiche restano attivi.
    app.setQuitOnLastWindowClosed(false);

    // Senza un tema di piattaforma configurato Qt conosce solo "hicolor", che
    // manca di molte icone delle app. Usa [appearance] icon_theme se impostato,
    // altrimenti il primo tema noto che sia davvero installato.
    {
        QStringList candidates;
        const QString configured = nicoos::ConfigManager::instance().iconTheme();
        if (!configured.isEmpty()) candidates << configured;
        candidates << "Papirus-Dark" << "Papirus" << "Adwaita" << "breeze" << "hicolor";
        const QStringList roots = QIcon::themeSearchPaths();
        for (const QString& name : std::as_const(candidates)) {
            bool found = false;
            for (const QString& root : roots) {
                if (QDir(root + "/" + name).exists()) { found = true; break; }
            }
            if (found) { QIcon::setThemeName(name); break; }
        }
    }

    nicoos::ShellApplication shell;
    shell.start();

    return app.exec();
}
