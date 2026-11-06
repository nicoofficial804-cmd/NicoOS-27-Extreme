#pragma once

#include <QQuickView>
#include <QList>
#include <QString>

namespace nicoos {

enum class LayerAnchor {
    Top = 1 << 0,
    Bottom = 1 << 1,
    Left = 1 << 2,
    Right = 1 << 3,
};

enum class ShellLayer { Background, Bottom, Top, Overlay };

enum class KeyboardMode {
    None,       // non prende mai il focus da tastiera (pannello, notifiche)
    OnDemand,   // focus al clic (centro di controllo: campi password)
    Exclusive,  // cattura la tastiera mentre è visibile (launcher, menu alimentazione)
};

// Finestra di base per ogni elemento dell'interfaccia della shell. Con
// LayerShellQt disponibile ogni finestra diventa una vera superficie
// wlr-layer-shell (ancorata, con exclusive zone, impilata sopra o sotto le
// finestre normali); senza, la finestra degrada a una normale finestra di
// primo livello senza cornice.
//
// Ogni istanza espone due proprietà di contesto QML:
//   surface — questa finestra (chiama surface.hideSurface() dal QML)
//   shell   — lo ShellController dell'intero processo
class LayerSurfaceWindow : public QQuickView {
    Q_OBJECT

public:
    explicit LayerSurfaceWindow(ShellLayer layer, QWindow* parent = nullptr);
    ~LayerSurfaceWindow() override;

    void setAnchors(int anchorFlags);
    void setExclusiveZone(int pixels);
    void setMargins(int top, int right, int bottom, int left);
    void setKeyboardMode(KeyboardMode mode);
    // Comodità legacy: true -> OnDemand, false -> None.
    void setKeyboardInteractive(bool interactive);

    // I popup si escludono a vicenda: mostrarne uno nasconde gli altri.
    void setExclusivePopup(bool on);
    static void hideAllPopups();

    // Configura il ruolo del layer, poi rende visibile la finestra.
    void showAsLayerSurface();
    void applyLayerShellRole();

    Q_INVOKABLE void hideSurface();

private:
    ShellLayer m_layer;
    int m_anchors{0};
    int m_exclusiveZone{0};
    int m_marginTop{0}, m_marginRight{0}, m_marginBottom{0}, m_marginLeft{0};
    KeyboardMode m_keyboardMode{KeyboardMode::None};
    bool m_exclusivePopup{false};
};

} // namespace nicoos
