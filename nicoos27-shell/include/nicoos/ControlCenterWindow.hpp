#pragma once

#include <QObject>
#include <memory>

namespace nicoos {

class NetworkManagerClient;
class AudioClient;
class BrightnessClient;
class BluetoothClient;
class PowerActions;
class WifiScanner;
class QuickActions;
class MediaClient;
class PopupWindow;

// Pannello a scomparsa accanto alla barra: scheda multimediale, tessere di
// attivazione rapida, elenco Wi-Fi, volume, luminosità, Bluetooth e pulsanti di
// sessione. L'accesso ai dispositivi vive nelle classi *Client; questa classe
// le collega al QML.
class ControlCenterWindow : public QObject {
    Q_OBJECT

public:
    ControlCenterWindow(QuickActions* quick, MediaClient* media, QObject* parent = nullptr);
    ~ControlCenterWindow() override;

    void toggle();

private:
    std::unique_ptr<NetworkManagerClient> m_network;
    std::unique_ptr<AudioClient> m_audio;
    std::unique_ptr<BrightnessClient> m_brightness;
    std::unique_ptr<BluetoothClient> m_bluetooth;
    std::unique_ptr<PowerActions> m_power;
    std::unique_ptr<WifiScanner> m_wifi;
    std::unique_ptr<PopupWindow> m_popup;
};

} // namespace nicoos
