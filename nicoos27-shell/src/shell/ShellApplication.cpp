#include "nicoos/ShellApplication.hpp"
#include "nicoos/ClipboardManager.hpp"
#include "nicoos/ConfigManager.hpp"
#include "nicoos/ControlCenterWindow.hpp"
#include "nicoos/HyprlandEventListener.hpp"
#include "nicoos/HyprlandIPC.hpp"
#include "nicoos/LauncherWindow.hpp"
#include "nicoos/LayerSurfaceWindow.hpp"
#include "nicoos/MediaClient.hpp"
#include "nicoos/NotificationDaemon.hpp"
#include "nicoos/PanelWindow.hpp"
#include "nicoos/PopupWindow.hpp"
#include "nicoos/PowerActions.hpp"
#include "nicoos/QuickActions.hpp"
#include "nicoos/ShellController.hpp"
#include "nicoos/WallpaperManager.hpp"
#include "nicoos/WorkspaceOverview.hpp"

#include <QDBusConnection>
#include <QDebug>
#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QQmlContext>
#include <QStandardPaths>

namespace nicoos {

ShellApplication::ShellApplication(QObject* parent) : QObject(parent) {
    ConfigManager::instance().watchForChanges();
}

ShellApplication::~ShellApplication() = default;

void ShellApplication::start() {
    if (!HyprlandIPC::instance().isAvailable()) {
        qWarning() << "[ShellApplication] HYPRLAND_INSTANCE_SIGNATURE non impostata."
                      " La shell NicoOS e' pensata per girare dentro una sessione Hyprland;"
                      " fino ad allora le funzioni workspace/finestre restano inattive.";
    }

    createModules();
    createPopups();
    wireRouting();
    wireControlSocket();

    m_wallpapers->restore();
    m_panel->show();

    qInfo() << "[ShellApplication] shell NicoOS 27 Extreme avviata.";
}

void ShellApplication::createModules() {
    m_hyprEvents = std::make_unique<HyprlandEventListener>();
    m_hyprEvents->start();

    m_quick = std::make_unique<QuickActions>();
    m_media = std::make_unique<MediaClient>();
    m_powerActions = std::make_unique<PowerActions>();
    // Prima gli errori sparivano in silenzio (il menu di spegnimento
    // e' gia' chiuso a quel punto).
    connect(m_powerActions.get(), &PowerActions::actionFailed, this,
            [](const QString& action, const QString& reason) {
                qWarning() << "[ShellApplication] azione di alimentazione non riuscita:" << action << reason;
            });
    m_wallpapers = std::make_unique<WallpaperManager>();
    m_clipboard = std::make_unique<ClipboardManager>();

    m_panel = std::make_unique<PanelWindow>(m_hyprEvents.get(), m_quick.get());
    m_launcher = std::make_unique<LauncherWindow>();
    m_controlCenter = std::make_unique<ControlCenterWindow>(m_quick.get(), m_media.get());
    m_workspaceOverview = std::make_unique<WorkspaceOverview>(m_hyprEvents.get());

    m_notifications = std::make_unique<NotificationDaemon>();
    m_notifications->registerService();
    connect(m_quick.get(), &QuickActions::doNotDisturbChanged, this, [this] {
        m_notifications->setSuppressPopups(m_quick->doNotDisturb());
    });
}

void ShellApplication::createPopups() {
    // Menu di spegnimento: overlay a tutto schermo che cattura la tastiera.
    m_powerMenu = std::make_unique<PopupWindow>(QUrl("qrc:/nicoos/qml/PowerMenu.qml"),
                                                PopupPlacement::Fullscreen, QSize(),
                                                KeyboardMode::Exclusive);
    m_powerMenu->context()->setContextProperty("powerActions", m_powerActions.get());
    m_powerMenu->load();

    // Selettore di sfondi.
    m_wallpaperPicker = std::make_unique<PopupWindow>(QUrl("qrc:/nicoos/qml/Wallpapers.qml"),
                                                      PopupPlacement::Center, QSize(920, 600),
                                                      KeyboardMode::Exclusive);
    m_wallpaperPicker->context()->setContextProperty("wallpapers", m_wallpapers.get());
    m_wallpaperPicker->context()->setContextProperty("wallpaperModel", m_wallpapers->model());
    m_wallpaperPicker->load();
    connect(m_wallpaperPicker.get(), &PopupWindow::aboutToShow,
            m_wallpapers.get(), &WallpaperManager::refresh);

    // Cronologia degli appunti.
    m_clipboardPopup = std::make_unique<PopupWindow>(QUrl("qrc:/nicoos/qml/Clipboard.qml"),
                                                     PopupPlacement::Center, QSize(520, 580),
                                                     KeyboardMode::Exclusive);
    m_clipboardPopup->context()->setContextProperty("clipboard", m_clipboard.get());
    m_clipboardPopup->load();
    connect(m_clipboardPopup.get(), &PopupWindow::aboutToShow,
            m_clipboard.get(), &ClipboardManager::refresh);

    // Calendario sotto l'orologio.
    m_calendar = std::make_unique<PopupWindow>(QUrl("qrc:/nicoos/qml/Calendar.qml"),
                                               PopupPlacement::PanelRight, QSize(330, 420),
                                               KeyboardMode::None);
    m_calendar->load();
}

void ShellApplication::wireRouting() {
    m_togglers["launcher"] = [this] { m_launcher->toggle(); };
    m_togglers["controlcenter"] = [this] { m_controlCenter->toggle(); };
    m_togglers["workspaces"] = [this] { m_workspaceOverview->toggle(); };
    m_togglers["power"] = [this] { m_powerMenu->toggle(); };
    m_togglers["wallpapers"] = [this] { m_wallpaperPicker->toggle(); };
    m_togglers["clipboard"] = [this] { m_clipboardPopup->toggle(); };
    m_togglers["calendar"] = [this] { m_calendar->toggle(); };

    auto& controller = ShellController::instance();
    connect(&controller, &ShellController::toggleRequested, this, [this](const QString& name) {
        if (const auto it = m_togglers.constFind(name); it != m_togglers.constEnd()) {
            it.value()();
        }
    });
    connect(&controller, &ShellController::actionRequested, this,
            [this](const QString& action) { handleAction(action); });
    connect(&controller, &ShellController::hideAllRequested, this,
            [] { LayerSurfaceWindow::hideAllPopups(); });
}

void ShellApplication::handleAction(const QString& action) {
    if (m_quick->runAction(action)) return;
    if (action == "wallpaper.random") {
        m_wallpapers->applyRandom();
        return;
    }
    if (action == "lock" || action == "suspend") {
        m_powerActions->trigger(action);
        return;
    }
    qWarning() << "[ShellApplication] azione sconosciuta:" << action;
}

void ShellApplication::wireControlSocket() {
    // NicoOS non cattura da sola le scorciatoie globali (entrerebbe in
    // conflitto con la gestione dell'input di Hyprland). hyprland.conf
    // lega i tasti a chiamate `socat` su questo socket; ogni riga e' un
    // comando:
    //   <popup>.toggle   launcher | controlcenter | workspaces | power |
    //                    wallpapers | clipboard | calendar
    //   <action>         dnd.toggle, nightlight.toggle, caffeine.toggle,
    //                    profile.cycle, screenshot.region, screenshot.full,
    //                    record.toggle, wallpaper.random, lock, suspend
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    const QString sockDir = runtimeDir + "/nicoos";
    QDir().mkpath(sockDir);
    const QString sockPath = sockDir + "/shell-control.sock";

    QLocalServer::removeServer(sockPath);
    auto* server = new QLocalServer(this);

    connect(server, &QLocalServer::newConnection, this, [this, server] {
        while (auto* conn = server->nextPendingConnection()) {
            connect(conn, &QLocalSocket::readyRead, this, [this, conn] {
                const QStringList lines =
                    QString::fromUtf8(conn->readAll()).split('\n', Qt::SkipEmptyParts);
                for (const QString& raw : lines) {
                    const QString cmd = raw.trimmed();
                    if (cmd.endsWith(".toggle")) {
                        const QString name = cmd.chopped(7);
                        if (const auto it = m_togglers.constFind(name); it != m_togglers.constEnd()) {
                            it.value()();
                            continue;
                        }
                    }
                    handleAction(cmd);
                }
                conn->disconnectFromServer();
            });
        }
    });

    if (!server->listen(sockPath)) {
        qWarning() << "[ShellApplication] impossibile aprire il socket di controllo in" << sockPath;
    }
}

} // namespace nicoos
