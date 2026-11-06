#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonArray>
#include <QJsonObject>
#include <optional>

namespace nicoos {

struct HyprWorkspace {
    int id{};
    QString name;
    QString monitor;
    int windowCount{};
    bool hasFullscreen{};
};

struct HyprWindow {
    QString address;
    QString cls;
    QString title;
    int workspaceId{};
    bool floating{};
    bool fullscreen{};
};

// Sottile wrapper sincrono attorno all'IPC su socket Unix di Hyprland
// (socket delle richieste equivalente a hyprctl, ".socket.sock").
// Questo è l'UNICO punto del codice che parla direttamente con Hyprland; ogni
// altro modulo passa da questa classe invece di invocare hyprctl o toccare il
// socket stesso.
class HyprlandIPC : public QObject {
    Q_OBJECT

public:
    static HyprlandIPC& instance();

    // Vero quando è stata trovata un'istanza di Hyprland in esecuzione.
    // Rivalutato a ogni chiamata, così una shell che sopravvive (o precede) un
    // riavvio di Hyprland, o partita con una HYPRLAND_INSTANCE_SIGNATURE
    // vecchia, trova quella viva.
    bool isAvailable();

    // Ambiente del compositore vivo, per i processi che avviamo (bloccanti,
    // hyprctl). Vuoto / 0 quando sconosciuto.
    QString instanceSignature();
    QString waylandDisplay();
    qint64 compositorPid();
    static QString runtimeDir();

    QVector<HyprWorkspace> activeWorkspaces();
    QVector<HyprWindow> clients();
    std::optional<HyprWorkspace> activeWorkspace();
    std::optional<HyprWindow> activeWindow();

    // Dispatch senza attesa di risposta, ad esempio "workspace 3" o
    // "movetoworkspace 2".
    bool dispatch(const QString& dispatcher, const QString& args = QString());

    bool switchToWorkspace(int id);
    bool moveActiveWindowToWorkspace(int id);
    bool moveWindowToWorkspace(const QString& windowAddress, int id);

private:
    explicit HyprlandIPC(QObject* parent = nullptr);

    QString socketPath();
    QString eventSocketPath();
    void refreshInstance();
    friend class HyprlandEventListener;

    // Invia una richiesta grezza sul ".socket.sock" e restituisce la risposta
    // grezza.
    QByteArray request(const QString& command);
    QJsonArray requestJsonArray(const QString& command);
    QJsonObject requestJsonObject(const QString& command);

    QString m_instanceSignature;
    QString m_waylandDisplay;
    qint64 m_pid{0};
    bool m_available{false};
};

} // namespace nicoos
