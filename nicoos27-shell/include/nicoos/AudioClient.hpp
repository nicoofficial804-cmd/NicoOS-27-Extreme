#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

class QProcess;

namespace nicoos {

// Volume e mute del sink predefinito tramite `pactl` (funziona con il server
// pulse di PipeWire e con PulseAudio). Le notifiche di cambiamento arrivano da
// un unico `pactl subscribe` sempre in esecuzione, quindi nulla fa polling.
//
// Trascinare uno slider genera molte chiamate setVolume(). Vengono accorpate in
// un solo `pactl set-sink-volume` alla volta (sempre con l'ultimo valore), e
// gli eventi di subscribe che arrivano subito dopo una nostra modifica vengono
// ignorati, così lo slider non torna mai a un valore vecchio.
class AudioClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(int volume READ volume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMuted NOTIFY volumeChanged)
    Q_PROPERTY(bool available READ available CONSTANT)

public:
    explicit AudioClient(QObject* parent = nullptr);
    ~AudioClient() override;

    int volume() const { return m_volume; }
    bool isMuted() const { return m_muted; }
    bool available() const { return m_available; }

    Q_INVOKABLE void setVolume(int percent);
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void refresh();

signals:
    void volumeChanged();

private:
    void startSubscriber();
    void onSubscribeOutput();
    void pump();
    void applyState(int volume, bool muted);

    QProcess* m_subscribe{nullptr};
    QTimer m_refreshDebounce;
    QElapsedTimer m_lastLocalChange;
    bool m_available{false};
    bool m_setRunning{false};
    bool m_refreshing{false};
    int m_wanted{-1};
    int m_volume{0};
    bool m_muted{false};
};

} // namespace nicoos
