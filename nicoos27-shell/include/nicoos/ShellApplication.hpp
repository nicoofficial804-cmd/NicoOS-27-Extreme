#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <functional>
#include <memory>

namespace nicoos {

class PanelWindow;
class LauncherWindow;
class ControlCenterWindow;
class WorkspaceOverview;
class NotificationDaemon;
class HyprlandEventListener;
class QuickActions;
class MediaClient;
class PowerActions;
class WallpaperManager;
class ClipboardManager;
class PopupWindow;

// Radice della composizione. Possiede ogni superficie e ogni backend della
// shell, e instrada verso di loro le richieste provenienti dal QML
// (ShellController) e dal socket di controllo locale (le scorciatoie in
// hyprland.conf).
class ShellApplication : public QObject {
    Q_OBJECT

public:
    explicit ShellApplication(QObject* parent = nullptr);
    ~ShellApplication() override;

    void start();

private:
    void createModules();
    void createPopups();
    void wireRouting();
    void wireControlSocket();
    void handleAction(const QString& action);

    std::unique_ptr<HyprlandEventListener> m_hyprEvents;
    std::unique_ptr<QuickActions> m_quick;
    std::unique_ptr<MediaClient> m_media;
    std::unique_ptr<PowerActions> m_powerActions;
    std::unique_ptr<WallpaperManager> m_wallpapers;
    std::unique_ptr<ClipboardManager> m_clipboard;

    std::unique_ptr<PanelWindow> m_panel;
    std::unique_ptr<LauncherWindow> m_launcher;
    std::unique_ptr<ControlCenterWindow> m_controlCenter;
    std::unique_ptr<WorkspaceOverview> m_workspaceOverview;
    std::unique_ptr<NotificationDaemon> m_notifications;

    std::unique_ptr<PopupWindow> m_powerMenu;
    std::unique_ptr<PopupWindow> m_wallpaperPicker;
    std::unique_ptr<PopupWindow> m_clipboardPopup;
    std::unique_ptr<PopupWindow> m_calendar;

    QHash<QString, std::function<void()>> m_togglers;
};

} // namespace nicoos
