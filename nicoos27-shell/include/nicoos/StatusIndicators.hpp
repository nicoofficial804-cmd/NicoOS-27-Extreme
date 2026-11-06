#pragma once

#include <QObject>
#include <QString>

namespace nicoos {

class NetworkManagerClient;
class AudioClient;
class SystemMonitor;

// Sottile aggregatore esposto a Panel.qml come unica proprietà di contesto,
// così il pannello non deve collegare tre oggetti client separati solo per
// disegnare glifi delle icone e tooltip. Ogni client sottostante possiede
// comunque la propria connessione D-Bus/sottoprocesso; questa classe si limita
// a inoltrare i loro segnali in un unico segnale "stato cambiato" per
// comodità.
class StatusIndicators : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString networkGlyph READ networkGlyph NOTIFY changed)
    Q_PROPERTY(QString audioGlyph READ audioGlyph NOTIFY changed)
    Q_PROPERTY(QString batteryGlyph READ batteryGlyph NOTIFY changed)

public:
    StatusIndicators(NetworkManagerClient* network, AudioClient* audio,
                      SystemMonitor* monitor, QObject* parent = nullptr);

    QString networkGlyph() const;
    QString audioGlyph() const;
    QString batteryGlyph() const;

signals:
    void changed();

private:
    NetworkManagerClient* m_network;
    AudioClient* m_audio;
    SystemMonitor* m_monitor;
};

} // namespace nicoos
