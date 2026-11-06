#pragma once

#include "nicoos/LayerSurfaceWindow.hpp"
#include "nicoos/WorkspaceOverview.hpp"  // per WorkspaceListModel
#include <QObject>
#include <memory>

namespace nicoos {

class HyprlandEventListener;
class NetworkManagerClient;
class AudioClient;
class QuickActions;
class TaskbarModel;

// La barra sempre visibile (in alto o in basso, secondo la configurazione):
// pulsante di avvio, punti delle aree di lavoro, lista finestre in stile
// Windows, icone di stato, orologio e pulsante di alimentazione. Pura
// presentazione; i dati dal vivo arrivano da SystemMonitor, HyprlandIPC,
// TaskbarModel e dalle classi *Client tramite proprietà di contesto QML.
class PanelWindow : public QObject {
    Q_OBJECT

public:
    PanelWindow(HyprlandEventListener* events, QuickActions* quick, QObject* parent = nullptr);
    ~PanelWindow() override;

    void show();

private:
    void buildWindow();

    HyprlandEventListener* m_events;
    QuickActions* m_quick;
    std::unique_ptr<WorkspaceListModel> m_workspaceModel;
    std::unique_ptr<TaskbarModel> m_taskbar;
    std::unique_ptr<LayerSurfaceWindow> m_window;
};

} // namespace nicoos
