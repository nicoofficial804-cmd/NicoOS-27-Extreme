#include "nicoos/PopupWindow.hpp"
#include "nicoos/ConfigManager.hpp"

#include <QGuiApplication>
#include <QQmlContext>
#include <QRect>
#include <QScreen>
#include <algorithm>

namespace nicoos {

PopupWindow::PopupWindow(const QUrl& qml, PopupPlacement placement, const QSize& size,
                         KeyboardMode keyboard, QObject* parent)
    : QObject(parent), m_qml(qml), m_placement(placement), m_size(size), m_keyboard(keyboard) {
    m_window = std::make_unique<LayerSurfaceWindow>(ShellLayer::Overlay);
    m_window->setExclusivePopup(true);
    m_window->setKeyboardMode(keyboard);

    auto& cfg = ConfigManager::instance();
    m_window->rootContext()->setContextProperty("accentColor", cfg.accentColor());
    m_window->rootContext()->setContextProperty("animSpeedMs", cfg.animationSpeedMs());

    connect(m_window.get(), &QWindow::visibleChanged, this, &PopupWindow::visibilityChanged);
}

PopupWindow::~PopupWindow() = default;

QQmlContext* PopupWindow::context() const {
    return m_window->rootContext();
}

void PopupWindow::load() {
    m_window->setSource(m_qml);
}

bool PopupWindow::isVisible() const {
    return m_window->isVisible();
}

void PopupWindow::setExclusive(bool on) {
    m_window->setExclusivePopup(on);
}

void PopupWindow::toggle() {
    if (isVisible()) hide(); else show();
}

void PopupWindow::hide() {
    m_window->setVisible(false);
}

void PopupWindow::show() {
    emit aboutToShow();

    auto& cfg = ConfigManager::instance();
    const QScreen* screen = QGuiApplication::primaryScreen();
    const QRect geo = screen ? screen->geometry() : QRect(0, 0, 1920, 1080);
    const bool panelTop = cfg.panelPosition() == PanelPosition::Top;
    const int edge = cfg.panelHeight() + 8;

    const int top = static_cast<int>(LayerAnchor::Top);
    const int bottom = static_cast<int>(LayerAnchor::Bottom);
    const int left = static_cast<int>(LayerAnchor::Left);
    const int right = static_cast<int>(LayerAnchor::Right);

    QSize size = m_size;
    int anchors = 0;
    int mt = 0, mr = 0, mb = 0, ml = 0;
    int zone = 0;

    switch (m_placement) {
    case PopupPlacement::Center:
        size = size.boundedTo(QSize(int(geo.width() * 0.96), int(geo.height() * 0.94)));
        break;
    case PopupPlacement::PanelRight:
    case PopupPlacement::PanelLeft: {
        const bool isRight = (m_placement == PopupPlacement::PanelRight);
        anchors = (panelTop ? top : bottom) | (isRight ? right : left);
        if (panelTop) mt = edge; else mb = edge;
        if (isRight) mr = 14; else ml = 14;
        size.setHeight(std::min(size.height(), geo.height() - edge - 24));
        break;
    }
    case PopupPlacement::Fullscreen:
        anchors = top | bottom | left | right;
        size = geo.size();
        zone = -1;  // ignora le zone esclusive delle altre superfici
        break;
    }

    m_window->setAnchors(anchors);
    m_window->setMargins(mt, mr, mb, ml);
    m_window->setExclusiveZone(zone);
    m_window->resize(size);
    m_window->showAsLayerSurface();
    if (m_keyboard != KeyboardMode::None) m_window->requestActivate();
}

} // namespace nicoos
