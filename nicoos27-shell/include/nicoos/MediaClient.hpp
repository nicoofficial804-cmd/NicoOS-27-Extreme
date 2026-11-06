#pragma once

#include <QObject>
#include <QString>

class QProcess;

namespace nicoos {

// Informazioni sul brano in riproduzione e controlli di trasporto per
// qualsiasi lettore MPRIS (Spotify, Firefox, mpv, VLC, ...) tramite
// `playerctl`. Un unico processo `playerctl --follow` sempre in esecuzione
// spinge i cambiamenti, quindi non c'è polling.
class MediaClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool hasPlayer READ hasPlayer NOTIFY changed)
    Q_PROPERTY(bool playing READ playing NOTIFY changed)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString artist READ artist NOTIFY changed)

public:
    explicit MediaClient(QObject* parent = nullptr);
    ~MediaClient() override;

    bool available() const { return m_available; }
    bool hasPlayer() const { return m_hasPlayer; }
    bool playing() const { return m_playing; }
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }

    Q_INVOKABLE void playPause();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();

signals:
    void changed();

private:
    void startFollower();
    void onReadyRead();

    QProcess* m_proc{nullptr};
    QByteArray m_buffer;
    bool m_available{false};
    bool m_hasPlayer{false};
    bool m_playing{false};
    QString m_title;
    QString m_artist;
};

} // namespace nicoos
