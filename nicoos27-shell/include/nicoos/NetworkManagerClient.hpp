#pragma once

#include <QDBusObjectPath>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantMap>

namespace nicoos {

enum class NetworkKind { None, Ethernet, WiFi, Unknown };

// Parla con NetworkManager via D-Bus (org.freedesktop.NetworkManager).
// Si iscrive ai segnali StateChanged / PropertiesChanged invece di fare
// polling, così pannello e centro di controllo riflettono subito i cambiamenti
// di connettività senza alcun costo in inattività. Le raffiche di segnali
// (NetworkManager diventa loquace durante il roaming) vengono accorpate in un
// solo aggiornamento.
//
// L'elenco delle reti vicine vive in WifiScanner (nmcli); questa classe
// descrive soltanto la connessione *corrente*.
class NetworkManagerClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY stateChanged)
    Q_PROPERTY(QString ssid READ currentSsid NOTIFY stateChanged)
    // 0-100; intensità reale del punto di accesso per il Wi-Fi, 100 per i
    // collegamenti cablati.
    Q_PROPERTY(int signalStrength READ signalStrength NOTIFY stateChanged)
    Q_PROPERTY(QString kindLabel READ kindLabel NOTIFY stateChanged)
    Q_PROPERTY(bool isWifi READ isWifi NOTIFY stateChanged)
    Q_PROPERTY(bool wifiEnabled READ wifiEnabled NOTIFY stateChanged)

public:
    explicit NetworkManagerClient(QObject* parent = nullptr);

    bool isConnected() const { return m_connected; }
    QString currentSsid() const { return m_ssid; }
    int signalStrength() const { return m_signalStrength; }
    bool isWifi() const { return m_kind == NetworkKind::WiFi; }
    bool wifiEnabled() const { return m_wifiEnabled; }
    QString kindLabel() const;

    // Chiede a ogni dispositivo Wi-Fi di scansionare (RequestScan).
    Q_INVOKABLE void requestRescan();
    // Accende o spegne la radio Wi-Fi ("WirelessEnabled" di NetworkManager).
    Q_INVOKABLE void toggleWifi(bool enabled);

signals:
    void stateChanged();

private slots:
    void refreshState();
    void scheduleRefresh();

private:
    QVariant getProperty(const QString& path, const QString& iface, const QString& name) const;
    void watchAccessPoint(const QString& apPath);
    void resetState();

    QTimer m_debounce;
    bool m_available{false};
    NetworkKind m_kind{NetworkKind::None};
    bool m_connected{false};
    bool m_wifiEnabled{true};
    QString m_ssid;
    int m_signalStrength{0};
    QString m_watchedAp;
};

} // namespace nicoos
