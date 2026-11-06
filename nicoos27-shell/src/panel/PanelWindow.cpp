#include "nicoos/PanelWindow.hpp"
#include "nicoos/AudioClient.hpp"
#include "nicoos/ConfigManager.hpp"
#include "nicoos/HyprlandEventListener.hpp"
#include "nicoos/HyprlandIPC.hpp"
#include "nicoos/NetworkManagerClient.hpp"
#include "nicoos/QuickActions.hpp"
#include "nicoos/SystemMonitor.hpp"
#include "nicoos/TaskbarModel.hpp"

#include <QGuiApplication>
#include <QQmlContext>
#include <QScreen>
#include <QTimer>

namespace nicoos {

PanelWindow::PanelWindow(HyprlandEventListener* events, QuickActions* quick, QObject* parent)
    : QObject(parent), m_events(events), m_quick(quick) {
    buildWindow();
}

PanelWindow::~PanelWindow() = default;

void PanelWindow::buildWindow() {
    m_window = std::make_unique<LayerSurfaceWindow>(ShellLayer::Top);

    auto& cfg = ConfigManager::instance();

    auto* monitor = new SystemMonitor(m_window.get());
    monitor->start(2000);

    // Client di stato locali al pannello (economici: un watch D-Bus e
    // un `pactl subscribe`), indipendenti da quelli del centro di controllo.
    auto* network = new NetworkManagerClient(m_window.get());
    auto* audio = new AudioClient(m_window.get());

    m_workspaceModel = std::make_unique<WorkspaceListModel>();
    // Una singola azione dell'utente (aprire una finestra, cambiare
    // workspace) genera diversi eventi Hyprland in pochi millisecondi.
    // Ogni aggiornamento costa chiamate IPC bloccanti, quindi raggruppa
    // la raffica in una sola.
    auto* wsDebounce = new QTimer(m_window.get());
    wsDebounce->setSingleShot(true);
    wsDebounce->setInterval(30);
    connect(wsDebounce, &QTimer::timeout, m_workspaceModel.get(), &WorkspaceListModel::refresh);
    auto scheduleRefresh = [wsDebounce] { wsDebounce->start(); };
    connect(m_events, &HyprlandEventListener::workspaceChanged, m_window.get(), scheduleRefresh);
    connect(m_events, &HyprlandEventListener::windowOpened, m_window.get(), scheduleRefresh);
    connect(m_events, &HyprlandEventListener::windowClosed, m_window.get(), scheduleRefresh);
    connect(m_events, &HyprlandEventListener::windowMoved, m_window.get(), scheduleRefresh);

    m_taskbar = std::make_unique<TaskbarModel>(m_events);

    auto* ctx = m_window->rootContext();
    ctx->setContextProperty("sysMonitor", monitor);
    ctx->setContextProperty("workspaceModel", m_workspaceModel.get());
    ctx->setContextProperty("taskbarModel", m_taskbar.get());
    ctx->setContextProperty("networkClient", network);
    ctx->setContextProperty("audioClient", audio);
    ctx->setContextProperty("quick", m_quick);
    ctx->setContextProperty("accentColor", cfg.accentColor());
    ctx->setContextProperty("panelOpacity", cfg.panelOpacity());
    ctx->setContextProperty("clockFormat", cfg.clockFormat());
    ctx->setContextProperty("panelHeightPx", cfg.panelHeight());

    m_window->setSource(QUrl("qrc:/nicoos/qml/Panel.qml"));

    // Rispetta [panel] position = top|bottom. Con bottom si ottiene la
    // classica barra delle applicazioni in stile Windows.
    const bool top = cfg.panelPosition() == PanelPosition::Top;
    const int edge = static_cast<int>(top ? LayerAnchor::Top : LayerAnchor::Bottom);
    m_window->setAnchors(edge | static_cast<int>(LayerAnchor::Left) |
                         static_cast<int>(LayerAnchor::Right));
    m_window->setExclusiveZone(cfg.panelHeight());
    m_window->setMargins(0, 0, 0, 0);
    m_window->setKeyboardMode(KeyboardMode::None);

    int width = 1920;
    if (const auto* screen = QGuiApplication::primaryScreen()) {
        width = screen->geometry().width();
    }
    m_window->resize(width, cfg.panelHeight());
}

void PanelWindow::show() {
    m_window->showAsLayerSurface();
}

} // namespace nicoos
