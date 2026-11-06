#include "nicoos/BrightnessClient.hpp"
#include "nicoos/ProcessUtil.hpp"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <algorithm>
#include <cmath>

namespace nicoos {

BrightnessClient::BrightnessClient(QObject* parent, const QString& backlightRoot) : QObject(parent) {
    detectDevice(backlightRoot);
    if (m_available) m_percent = std::clamp(int(std::lround(100.0 * readRaw() / m_max)), 0, 100);
}

void BrightnessClient::detectDevice(const QString& root) {
    const QDir dir(root);
    if (!dir.exists()) return;  // desktop fissi, la maggior parte delle VM

    // Preferisci i controller "raw" (intel_backlight, amdgpu_bl*) a quelli
    // "platform" e "firmware" (acpi_video*), come fa systemd.
    auto score = [](const QString& type) {
        if (type == "raw") return 3;
        if (type == "platform") return 2;
        return 1;
    };
    int bestScore = 0;
    for (const QString& name : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        const QString path = dir.filePath(name);
        QFile typeFile(path + "/type");
        QString type = "firmware";
        if (typeFile.open(QIODevice::ReadOnly)) type = QString::fromUtf8(typeFile.readAll()).trimmed();
        QFile maxFile(path + "/max_brightness");
        if (!maxFile.open(QIODevice::ReadOnly)) continue;
        const int max = maxFile.readAll().trimmed().toInt();
        if (max <= 0) continue;
        if (score(type) > bestScore) {
            bestScore = score(type);
            m_name = name;
            m_devicePath = path;
            m_max = max;
            m_available = true;
        }
    }
}

int BrightnessClient::readRaw() const {
    QFile f(m_devicePath + "/brightness");
    if (!f.open(QIODevice::ReadOnly)) return m_max;
    return f.readAll().trimmed().toInt();
}

void BrightnessClient::refresh() {
    if (!m_available || m_busy) return;
    const int percent = std::clamp(int(std::lround(100.0 * readRaw() / m_max)), 0, 100);
    if (percent != m_percent) {
        m_percent = percent;
        emit brightnessChanged();
    }
}

void BrightnessClient::setBrightnessPercent(int percent) {
    if (!m_available) return;
    percent = std::clamp(percent, 1, 100);
    if (percent != m_percent) {
        m_percent = percent;
        emit brightnessChanged();
    }
    m_wantedRaw = std::max(1, int(std::lround(m_max * (percent / 100.0))));
    pump();
}

// Una scrittura alla volta; dopo, applica il valore richiesto più recente.
void BrightnessClient::pump() {
    if (m_busy || m_wantedRaw < 0) return;
    const int raw = m_wantedRaw;
    m_wantedRaw = -1;
    m_busy = true;
    writeRaw(raw);
}

void BrightnessClient::finishWrite(bool ok) {
    m_busy = false;
    if (m_wantedRaw >= 0) {
        pump();
        return;
    }
    if (!ok) {
        // Non ha funzionato nulla: mostra il valore reale invece di uno
        // finto.
        const int percent = std::clamp(int(std::lround(100.0 * readRaw() / m_max)), 0, 100);
        m_percent = percent;
        emit brightnessChanged();
    }
}

void BrightnessClient::writeRaw(int raw) {
    // 1) scrittura diretta in sysfs
    const QString file = m_devicePath + "/brightness";
    if (QFileInfo(file).isWritable()) {
        QFile f(file);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream(&f) << raw;
            f.close();
            finishWrite(true);
            return;
        }
    }

    // 2) systemd-logind (consentito per la sessione attiva)
    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.freedesktop.login1", "/org/freedesktop/login1/session/auto",
        "org.freedesktop.login1.Session", "SetBrightness");
    msg << QStringLiteral("backlight") << m_name << static_cast<uint>(raw);
    auto* watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, raw](QDBusPendingCallWatcher* w) {
        const bool ok = !QDBusPendingReply<>(*w).isError();
        w->deleteLater();
        if (ok) {
            finishWrite(true);
            return;
        }
        // 3) brightnessctl
        if (hasExecutable("brightnessctl")) {
            runCapture(this, "brightnessctl", {"--device=" + m_name, "set", QString::number(raw)},
                       [this](int code, const QString&) { finishWrite(code == 0); });
        } else {
            finishWrite(false);
        }
    });
}

} // namespace nicoos
