#pragma once

#include "nicoos/LayerSurfaceWindow.hpp"

#include <QObject>
#include <QSize>
#include <QUrl>
#include <memory>

class QQmlContext;

namespace nicoos {

enum class PopupPlacement {
    Center,       // centrato sullo schermo
    PanelRight,   // accanto al pannello, allineato a destra (centro di controllo, calendario)
    PanelLeft,    // accanto al pannello, allineato a sinistra
    Fullscreen,   // copre l'intera uscita video (menu alimentazione)
};

// Un popup QML che vive su una propria layer surface. Uso:
//   auto* p = new PopupWindow(url, placement, size, keyboardMode);
//   p->context()->setContextProperty(...);   // PRIMA di load()
//   p->load();
class PopupWindow : public QObject {
    Q_OBJECT

public:
    PopupWindow(const QUrl& qml, PopupPlacement placement, const QSize& size,
                KeyboardMode keyboard, QObject* parent = nullptr);
    ~PopupWindow() override;

    QQmlContext* context() const;
    void load();
    bool isVisible() const;
    // Le pile di notifiche ecc. non devono chiudere gli altri popup.
    void setExclusive(bool on);

public slots:
    void toggle();
    void show();
    void hide();

signals:
    void aboutToShow();
    void visibilityChanged(bool visible);

private:
    QUrl m_qml;
    PopupPlacement m_placement;
    QSize m_size;
    KeyboardMode m_keyboard;
    std::unique_ptr<LayerSurfaceWindow> m_window;
};

} // namespace nicoos
