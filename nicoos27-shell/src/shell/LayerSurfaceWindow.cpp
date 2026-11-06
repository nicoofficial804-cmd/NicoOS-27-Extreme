#include "nicoos/LayerSurfaceWindow.hpp"
#include "nicoos/AppIconProvider.hpp"
#include "nicoos/ShellController.hpp"

#include <QGuiApplication>
#include <QMargins>
#include <QQmlContext>
#include <QQmlEngine>
#include <QDebug>

#if defined(NICOOS_HAVE_LAYER_SHELL)
#include <LayerShellQt/Window>
#endif

namespace nicoos {

namespace {
QList<LayerSurfaceWindow*>& popupRegistry() {
    static QList<LayerSurfaceWindow*> list;
    return list;
}

#if defined(NICOOS_HAVE_LAYER_SHELL)
LayerShellQt::Window::Layer toLayer(ShellLayer layer) {
    switch (layer) {
        case ShellLayer::Background: return LayerShellQt::Window::LayerBackground;
        case ShellLayer::Bottom:     return LayerShellQt::Window::LayerBottom;
        case ShellLayer::Top:        return LayerShellQt::Window::LayerTop;
        case ShellLayer::Overlay:    return LayerShellQt::Window::LayerOverlay;
    }
    return LayerShellQt::Window::LayerTop;
}

LayerShellQt::Window::Anchors toAnchors(int flags) {
    LayerShellQt::Window::Anchors result;
    if (flags & static_cast<int>(LayerAnchor::Top))    result |= LayerShellQt::Window::AnchorTop;
    if (flags & static_cast<int>(LayerAnchor::Bottom)) result |= LayerShellQt::Window::AnchorBottom;
    if (flags & static_cast<int>(LayerAnchor::Left))   result |= LayerShellQt::Window::AnchorLeft;
    if (flags & static_cast<int>(LayerAnchor::Right))  result |= LayerShellQt::Window::AnchorRight;
    return result;
}

LayerShellQt::Window::KeyboardInteractivity toKeyboard(KeyboardMode mode) {
    switch (mode) {
        case KeyboardMode::None:      return LayerShellQt::Window::KeyboardInteractivityNone;
        case KeyboardMode::OnDemand:  return LayerShellQt::Window::KeyboardInteractivityOnDemand;
        case KeyboardMode::Exclusive: return LayerShellQt::Window::KeyboardInteractivityExclusive;
    }
    return LayerShellQt::Window::KeyboardInteractivityNone;
}
#endif
} // namespace

LayerSurfaceWindow::LayerSurfaceWindow(ShellLayer layer, QWindow* parent)
    : QQuickView(parent), m_layer(layer) {
    setColor(Qt::transparent);
    setResizeMode(QQuickView::SizeRootObjectToView);

#if defined(NICOOS_HAVE_LAYER_SHELL)
    setFlags(Qt::FramelessWindowHint);
#else
    // Solo fallback: senza LayerShellQt sono normali finestre di primo livello.
    setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
#endif

    // Ogni superficie della shell ha il proprio QQmlEngine, quindi il
    // provider di icone viene registrato per finestra (economico: le
    // pixmap sono memorizzate in cache in modo pigro).
    engine()->addImageProvider("nicoosicon", new AppIconProvider());

    rootContext()->setContextProperty("surface", this);
    rootContext()->setContextProperty("shell", &ShellController::instance());
}

LayerSurfaceWindow::~LayerSurfaceWindow() {
    popupRegistry().removeAll(this);
}

void LayerSurfaceWindow::setAnchors(int anchorFlags) { m_anchors = anchorFlags; }
void LayerSurfaceWindow::setExclusiveZone(int pixels) { m_exclusiveZone = pixels; }

void LayerSurfaceWindow::setMargins(int top, int right, int bottom, int left) {
    m_marginTop = top;
    m_marginRight = right;
    m_marginBottom = bottom;
    m_marginLeft = left;
}

void LayerSurfaceWindow::setKeyboardMode(KeyboardMode mode) { m_keyboardMode = mode; }

void LayerSurfaceWindow::setKeyboardInteractive(bool interactive) {
    m_keyboardMode = interactive ? KeyboardMode::OnDemand : KeyboardMode::None;
}

void LayerSurfaceWindow::setExclusivePopup(bool on) {
    m_exclusivePopup = on;
    popupRegistry().removeAll(this);
    if (on) popupRegistry().append(this);
}

void LayerSurfaceWindow::hideAllPopups() {
    const auto list = popupRegistry();
    for (auto* w : list) {
        if (w->isVisible()) w->setVisible(false);
    }
}

void LayerSurfaceWindow::hideSurface() {
    setVisible(false);
}

void LayerSurfaceWindow::showAsLayerSurface() {
    if (m_exclusivePopup) {
        const auto list = popupRegistry();
        for (auto* w : list) {
            if (w != this && w->isVisible()) w->setVisible(false);
        }
    }
    // LayerShellQt riapplica il proprio ruolo ogni volta che Qt ricrea la
    // wl_surface (hide/show), quindi configurarla prima di show basta.
    applyLayerShellRole();
    setVisible(true);
}

void LayerSurfaceWindow::applyLayerShellRole() {
#if defined(NICOOS_HAVE_LAYER_SHELL)
    if (auto* ls = LayerShellQt::Window::get(this)) {
        ls->setLayer(toLayer(m_layer));
        ls->setAnchors(toAnchors(m_anchors));
        ls->setExclusiveZone(m_exclusiveZone);
        ls->setMargins(QMargins(m_marginLeft, m_marginTop, m_marginRight, m_marginBottom));
        ls->setKeyboardInteractivity(toKeyboard(m_keyboardMode));
        ls->setScope(QStringLiteral("nicoos-shell"));
    } else {
        qWarning() << "[LayerSurfaceWindow] LayerShellQt::Window::get() ha restituito null per" << this;
    }
#else
    static bool warnedOnce = false;
    if (!warnedOnce) {
        warnedOnce = true;
        qWarning() << "[LayerSurfaceWindow] compilato senza LayerShellQt; uso normali "
                      "finestre di primo livello. Installa 'layer-shell-qt' e riconfigura.";
    }
#endif
}

} // namespace nicoos
