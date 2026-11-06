#include "nicoos/ControlCenterWindow.hpp"
#include "nicoos/AudioClient.hpp"
#include "nicoos/BluetoothClient.hpp"
#include "nicoos/BrightnessClient.hpp"
#include "nicoos/MediaClient.hpp"
#include "nicoos/NetworkManagerClient.hpp"
#include "nicoos/PopupWindow.hpp"
#include "nicoos/PowerActions.hpp"
#include "nicoos/QuickActions.hpp"
#include "nicoos/WifiScanner.hpp"

#include <QQmlContext>

namespace nicoos {

ControlCenterWindow::ControlCenterWindow(QuickActions* quick, MediaClient* media, QObject* parent)
    : QObject(parent) {
    m_network = std::make_unique<NetworkManagerClient>();
    m_audio = std::make_unique<AudioClient>();
    m_brightness = std::make_unique<BrightnessClient>();
    m_bluetooth = std::make_unique<BluetoothClient>();
    m_power = std::make_unique<PowerActions>();
    m_wifi = std::make_unique<WifiScanner>();

    // Tastiera OnDemand: il campo password del Wi-Fi richiede di digitare, ma
    // il pannello non deve rubare la tastiera alle altre finestre prima del
    // clic.
    m_popup = std::make_unique<PopupWindow>(QUrl("qrc:/nicoos/qml/ControlCenter.qml"),
                                            PopupPlacement::PanelRight, QSize(380, 760),
                                            KeyboardMode::OnDemand);

    auto* ctx = m_popup->context();
    ctx->setContextProperty("networkClient", m_network.get());
    ctx->setContextProperty("audioClient", m_audio.get());
    ctx->setContextProperty("brightnessClient", m_brightness.get());
    ctx->setContextProperty("bluetoothClient", m_bluetooth.get());
    ctx->setContextProperty("powerActions", m_power.get());
    ctx->setContextProperty("wifi", m_wifi.get());
    ctx->setContextProperty("quick", quick);
    ctx->setContextProperty("media", media);
    m_popup->load();

    connect(m_popup.get(), &PopupWindow::aboutToShow, m_wifi.get(), &WifiScanner::scan);
    // Nel frattempo volume e luminosità possono essere cambiati con i tasti
    // hardware.
    connect(m_popup.get(), &PopupWindow::aboutToShow, m_audio.get(), &AudioClient::refresh);
    connect(m_popup.get(), &PopupWindow::aboutToShow, m_brightness.get(), &BrightnessClient::refresh);
}

ControlCenterWindow::~ControlCenterWindow() = default;

void ControlCenterWindow::toggle() {
    m_popup->toggle();
}

} // namespace nicoos
