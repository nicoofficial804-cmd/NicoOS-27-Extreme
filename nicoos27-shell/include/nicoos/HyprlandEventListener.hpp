#pragma once

#include <QObject>
#include <QLocalSocket>
#include <QByteArray>

namespace nicoos {

// Si collega al socket degli eventi di Hyprland (.socket2.sock) e riemette gli
// eventi analizzati come segnali Qt. È ciò che permette al pannello e alla
// panoramica delle aree di lavoro di aggiornarsi in modo reattivo invece di
// interrogare hyprctl a timer.
class HyprlandEventListener : public QObject {
    Q_OBJECT

public:
    explicit HyprlandEventListener(QObject* parent = nullptr);

    void start();

signals:
    void workspaceChanged(int id);
    void activeWindowChanged(const QString& windowClass, const QString& title);
    void windowOpened(const QString& address, const QString& windowClass);
    void windowClosed(const QString& address);
    void windowMoved(const QString& address, int workspaceId);
    void monitorChanged(const QString& monitorName);
    void configReloaded();
    // Collettore generico per i consumatori interessati agli eventi grezzi non
    // modellati sopra (evita che questa classe debba conoscere in anticipo ogni
    // tipo di evento di Hyprland).
    void rawEvent(const QString& name, const QString& payload);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void tryConnect();
    void handleLine(const QString& line);

    QLocalSocket m_socket;
    QByteArray m_buffer;
    int m_reconnectDelayMs{1000};
};

} // namespace nicoos
