#include "nicoos/MediaClient.hpp"
#include "nicoos/ProcessUtil.hpp"

#include <QProcess>
#include <QTimer>

namespace nicoos {

MediaClient::MediaClient(QObject* parent) : QObject(parent) {
    m_available = hasExecutable("playerctl");
    if (m_available) startFollower();
}

MediaClient::~MediaClient() {
    if (m_proc) {
        m_proc->disconnect(this);
        m_proc->kill();
        m_proc->waitForFinished(300);
    }
}

void MediaClient::startFollower() {
    m_proc = new QProcess(this);
    connect(m_proc, &QProcess::readyReadStandardOutput, this, &MediaClient::onReadyRead);
    connect(m_proc, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {
        m_proc->deleteLater();
        m_proc = nullptr;
        if (m_hasPlayer) {
            m_hasPlayer = false;
            m_playing = false;
            emit changed();
        }
        // In alcune versioni playerctl esce quando sparisce l'ultimo lettore:
        // riavvialo più tardi (economico, nessun ciclo attivo).
        QTimer::singleShot(5000, this, [this] { if (!m_proc) startFollower(); });
    });
    m_proc->start("playerctl", {"--follow", "metadata", "--format",
                                "{{status}}\t{{artist}}\t{{title}}"});
}

void MediaClient::onReadyRead() {
    m_buffer += m_proc->readAllStandardOutput();
    int nl;
    while ((nl = m_buffer.indexOf('\n')) != -1) {
        const QString line = QString::fromUtf8(m_buffer.left(nl));
        m_buffer.remove(0, nl + 1);

        const QStringList parts = line.split('\t');
        const QString status = parts.value(0).trimmed();
        const QString artist = parts.value(1).trimmed();
        const QString title = parts.mid(2).join('\t').trimmed();

        const bool has = (status == "Playing" || status == "Paused") || !title.isEmpty();
        const bool playing = (status == "Playing");
        if (has != m_hasPlayer || playing != m_playing || title != m_title || artist != m_artist) {
            m_hasPlayer = has;
            m_playing = playing;
            m_title = title;
            m_artist = artist;
            emit changed();
        }
    }
}

void MediaClient::playPause() { QProcess::startDetached("playerctl", {"play-pause"}); }
void MediaClient::next() { QProcess::startDetached("playerctl", {"next"}); }
void MediaClient::previous() { QProcess::startDetached("playerctl", {"previous"}); }

} // namespace nicoos
