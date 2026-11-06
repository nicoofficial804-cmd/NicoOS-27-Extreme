#include "nicoos/SystemMonitor.hpp"

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStringList>

namespace nicoos {

SystemMonitor::SystemMonitor(QObject* parent) : QObject(parent) {
    connect(&m_timer, &QTimer::timeout, this, &SystemMonitor::poll);
}

void SystemMonitor::start(int intervalMs) {
    // Inizializza subito la base per il delta della CPU, cosi' la prima
    // lettura emessa non e' una media "dall'avvio" priva di senso.
    readCpu();
    poll();
    m_timer.start(intervalMs);
}

void SystemMonitor::poll() {
    // Il pannello mostra percentuali intere, quindi risveglia QML solo
    // quando uno dei valori visualizzati cambia davvero. A riposo questo
    // elimina gran parte delle rivalutazioni dei binding che il polling
    // ogni 2 secondi provocava.
    const int cpuBefore = qRound(m_cpuUsage);
    const int memBefore = qRound(m_memUsage);
    const int batBefore = m_batteryPercent;
    const bool chargingBefore = m_batteryCharging;
    const bool hasBefore = m_hasBattery;

    readCpu();
    readMemory();
    readBattery();

    if (cpuBefore != qRound(m_cpuUsage) || memBefore != qRound(m_memUsage) ||
        batBefore != m_batteryPercent || chargingBefore != m_batteryCharging ||
        hasBefore != m_hasBattery) {
        emit statsUpdated();
    }
}

void SystemMonitor::readCpu() {
    QFile f("/proc/stat");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    QTextStream in(&f);
    const QString line = in.readLine();
    const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 8 || parts[0] != "cpu") {
        return;
    }

    quint64 user = parts[1].toULongLong();
    quint64 nice = parts[2].toULongLong();
    quint64 system = parts[3].toULongLong();
    quint64 idle = parts[4].toULongLong();
    quint64 iowait = parts[5].toULongLong();
    quint64 irq = parts[6].toULongLong();
    quint64 softirq = parts[7].toULongLong();

    quint64 idleAll = idle + iowait;
    quint64 total = user + nice + system + idleAll + irq + softirq;

    quint64 totalDelta = total - m_prevTotal;
    quint64 idleDelta = idleAll - m_prevIdle;

    if (totalDelta > 0) {
        m_cpuUsage = 100.0 * (1.0 - (double(idleDelta) / double(totalDelta)));
    }

    m_prevTotal = total;
    m_prevIdle = idleAll;
}

void SystemMonitor::readMemory() {
    QFile f("/proc/meminfo");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    QTextStream in(&f);
    quint64 total = 0, available = 0;

    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.startsWith("MemTotal:")) {
            total = line.split(' ', Qt::SkipEmptyParts).value(1).toULongLong();
        } else if (line.startsWith("MemAvailable:")) {
            available = line.split(' ', Qt::SkipEmptyParts).value(1).toULongLong();
        }
        if (total && available) break;
    }

    if (total > 0) {
        m_memUsage = 100.0 * (1.0 - (double(available) / double(total)));
    }
}

void SystemMonitor::readBattery() {
    // Cerca la prima batteria in /sys/class/power_supply
    // (BAT0, BAT1, ...). I sistemi senza batteria (desktop, la maggior
    // parte delle VM) riportano semplicemente hasBattery() == false e il
    // pannello nasconde l'indicatore.
    QDir psDir("/sys/class/power_supply");
    if (!psDir.exists()) {
        m_hasBattery = false;
        return;
    }

    for (const QString& entry : psDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!entry.startsWith("BAT")) {
            continue;
        }

        QFile capFile(psDir.filePath(entry + "/capacity"));
        QFile statusFile(psDir.filePath(entry + "/status"));

        if (capFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_batteryPercent = capFile.readAll().trimmed().toInt();
            m_hasBattery = true;
        }
        if (statusFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString status = statusFile.readAll().trimmed();
            m_batteryCharging = (status == "Charging" || status == "Full");
        }
        return;
    }

    m_hasBattery = false;
}

} // namespace nicoos
