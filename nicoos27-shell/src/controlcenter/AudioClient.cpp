#include "nicoos/AudioClient.hpp"
#include "nicoos/ProcessUtil.hpp"

#include <QProcess>
#include <QRegularExpression>
#include <algorithm>

namespace nicoos {

AudioClient::AudioClient(QObject* parent) : QObject(parent) {
    m_available = hasExecutable("pactl");

    m_refreshDebounce.setSingleShot(true);
    m_refreshDebounce.setInterval(80);
    connect(&m_refreshDebounce, &QTimer::timeout, this, &AudioClient::refresh);

    if (!m_available) return;
    refresh();
    startSubscriber();
}

AudioClient::~AudioClient() {
    if (m_subscribe) {
        m_subscribe->disconnect(this);
        m_subscribe->kill();
        m_subscribe->waitForFinished(300);
    }
}

void AudioClient::startSubscriber() {
    m_subscribe = new QProcess(this);
    connect(m_subscribe, &QProcess::readyReadStandardOutput, this, &AudioClient::onSubscribeOutput);
    connect(m_subscribe, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {
        m_subscribe->deleteLater();
        m_subscribe = nullptr;
        // Il server audio è stato riavviato (o è andato in crash): riconnetti
        // a breve.
        QTimer::singleShot(3000, this, [this] { if (!m_subscribe) startSubscriber(); });
    });
    m_subscribe->start("pactl", {"subscribe"});
}

void AudioClient::onSubscribeOutput() {
    const QByteArray data = m_subscribe->readAllStandardOutput();
    if (!(data.contains("sink") || data.contains("server"))) return;

    // Le nostre stesse modifiche tornano indietro come eventi; rileggere
    // mentre un trascinamento è in corso mostrerebbe un valore vecchio e
    // farebbe saltare indietro lo slider.
    const bool recentlyLocal = m_lastLocalChange.isValid() && m_lastLocalChange.elapsed() < 400;
    if (recentlyLocal || m_setRunning || m_wanted >= 0) return;

    m_refreshDebounce.start();
}

void AudioClient::applyState(int volume, bool muted) {
    if (volume == m_volume && muted == m_muted) return;
    m_volume = volume;
    m_muted = muted;
    emit volumeChanged();
}

void AudioClient::refresh() {
    if (!m_available || m_refreshing) return;
    m_refreshing = true;
    runCapture(this, "pactl", {"get-sink-volume", "@DEFAULT_SINK@"}, [this](int code, const QString& volOut) {
        if (code != 0) {
            m_refreshing = false;
            return;
        }
        static const QRegularExpression re(R"((\d+)%)");
        const auto match = re.match(volOut);
        const int volume = match.hasMatch() ? match.captured(1).toInt() : m_volume;

        runCapture(this, "pactl", {"get-sink-mute", "@DEFAULT_SINK@"},
                   [this, volume](int, const QString& muteOut) {
                       m_refreshing = false;
                       // Una modifica locale fatta mentre leggevamo ha la
                       // precedenza.
                       if (m_setRunning || m_wanted >= 0) return;
                       applyState(volume, muteOut.contains("yes"));
                   });
    });
}

void AudioClient::setVolume(int percent) {
    if (!m_available) return;
    percent = std::clamp(percent, 0, 150);
    m_lastLocalChange.restart();
    applyState(percent, m_muted);
    m_wanted = percent;
    pump();
}

// Esegue un solo `pactl set-sink-volume` alla volta; quando termina, applica il
// valore richiesto più recente se nel frattempo lo slider si è mosso.
void AudioClient::pump() {
    if (m_setRunning || m_wanted < 0) return;
    const int value = m_wanted;
    m_wanted = -1;
    m_setRunning = true;
    runCapture(this, "pactl", {"set-sink-volume", "@DEFAULT_SINK@", QString::number(value) + "%"},
               [this](int, const QString&) {
                   m_setRunning = false;
                   m_lastLocalChange.restart();
                   if (m_wanted >= 0) pump();
               });
}

void AudioClient::toggleMute() {
    if (!m_available) return;
    m_lastLocalChange.restart();
    applyState(m_volume, !m_muted);
    runCapture(this, "pactl", {"set-sink-mute", "@DEFAULT_SINK@", "toggle"},
               [this](int, const QString&) { m_lastLocalChange.restart(); });
}

} // namespace nicoos
