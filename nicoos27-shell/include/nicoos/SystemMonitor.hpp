#pragma once

#include <QObject>
#include <QTimer>

namespace nicoos {

// Interroga a intervalli modesti interfacce leggere del kernel (/proc, /sys)
// per statistiche che non hanno un'API del kernel a eventi (carico CPU,
// memoria, batteria). Lo stato di rete/audio/bluetooth viene ottenuto a parte
// tramite segnali D-Bus (NetworkManagerClient, AudioClient, BluetoothClient) e
// NON viene interrogato qui, dato che quelli hanno vere notifiche di
// cambiamento.
//
// L'intervallo di polling resta volutamente a bassa frequenza (2 s
// predefiniti) per mantenere trascurabile l'uso di CPU in inattività, secondo
// i requisiti di prestazioni della shell.
class SystemMonitor : public QObject {
    Q_OBJECT
    Q_PROPERTY(double cpuUsagePercent READ cpuUsagePercent NOTIFY statsUpdated)
    Q_PROPERTY(double memoryUsagePercent READ memoryUsagePercent NOTIFY statsUpdated)
    Q_PROPERTY(int batteryPercent READ batteryPercent NOTIFY statsUpdated)
    Q_PROPERTY(bool batteryCharging READ batteryCharging NOTIFY statsUpdated)
    Q_PROPERTY(bool hasBattery READ hasBattery NOTIFY statsUpdated)

public:
    explicit SystemMonitor(QObject* parent = nullptr);

    void start(int intervalMs = 2000);

    double cpuUsagePercent() const { return m_cpuUsage; }
    double memoryUsagePercent() const { return m_memUsage; }
    int batteryPercent() const { return m_batteryPercent; }
    bool batteryCharging() const { return m_batteryCharging; }
    bool hasBattery() const { return m_hasBattery; }

signals:
    void statsUpdated();

private slots:
    void poll();

private:
    void readCpu();
    void readMemory();
    void readBattery();

    QTimer m_timer;

    // Contatori cumulativi di /proc/stat del campione precedente, usati per
    // calcolare l'uso istantaneo della CPU come differenza invece che come
    // media cumulativa dall'avvio.
    quint64 m_prevIdle{0};
    quint64 m_prevTotal{0};

    double m_cpuUsage{0.0};
    double m_memUsage{0.0};
    int m_batteryPercent{100};
    bool m_batteryCharging{false};
    bool m_hasBattery{false};
};

} // namespace nicoos
