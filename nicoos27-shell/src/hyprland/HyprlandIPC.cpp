#include "nicoos/HyprlandIPC.hpp"

#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonValue>
#include <QProcessEnvironment>
#include <QFile>
#include <QFileInfo>
#include <unistd.h>
#include <QDir>
#include <QDebug>

namespace nicoos {

HyprlandIPC& HyprlandIPC::instance() {
    static HyprlandIPC inst;
    return inst;
}

HyprlandIPC::HyprlandIPC(QObject* parent) : QObject(parent) {
    refreshInstance();
    if (!m_available) {
        qWarning() << "[HyprlandIPC] nessuna istanza di Hyprland in esecuzione trovata; "
                      "fuori da Hyprland, IPC disabilitato (continuerò a cercare).";
    }
}

QString HyprlandIPC::runtimeDir() {
    QString dir = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (dir.isEmpty()) dir = QStringLiteral("/run/user/%1").arg(::getuid());
    return dir;
}

namespace {
struct LockInfo { qint64 pid{0}; QString display; };

// /run/user/N/hypr/<sig>/hyprland.lock: riga 1 = pid del compositore,
// riga 2 = il display Wayland che serve.
LockInfo readLock(const QString& sigDir) {
    LockInfo info;
    QFile f(sigDir + "/hyprland.lock");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        info.pid = f.readLine().trimmed().toLongLong();
        info.display = QString::fromUtf8(f.readLine().trimmed());
    }
    return info;
}
bool pidAlive(qint64 pid) { return pid > 0 && QFileInfo::exists(QStringLiteral("/proc/%1").arg(pid)); }
} // namespace

// Economica quando nulla è cambiato (uno stat); scansiona ~/hypr/* solo quando
// l'istanza in cache non c'è più. La signature ereditata dall'ambiente viene
// provata per prima ma mai creduta ciecamente: dopo un riavvio di Hyprland
// punta a una directory che non esiste più.
void HyprlandIPC::refreshInstance() {
    const QString base = runtimeDir() + "/hypr/";

    auto accept = [&](const QString& sig) {
        const QString dir = base + sig;
        if (!QFileInfo::exists(dir + "/.socket.sock")) return false;
        const LockInfo li = readLock(dir);
        if (li.pid > 0 && !pidAlive(li.pid)) return false;  // dir vecchia di un compositore morto
        m_instanceSignature = sig;
        m_pid = li.pid;
        m_waylandDisplay = li.display;
        m_available = true;
        return true;
    };

    if (m_available && accept(m_instanceSignature)) return;

    const QString envSig = qEnvironmentVariable("HYPRLAND_INSTANCE_SIGNATURE");
    if (!envSig.isEmpty() && accept(envSig)) return;

    // Vince l'istanza viva più recente.
    QDir d(base);
    const auto dirs = d.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    for (const QFileInfo& fi : dirs) {
        if (accept(fi.fileName())) {
            qInfo() << "[HyprlandIPC] uso l'istanza di Hyprland" << m_instanceSignature
                    << "(l'ambiente puntava a" << (envSig.isEmpty() ? "niente" : envSig) << ")";
            return;
        }
    }

    m_available = false;
    m_pid = 0;
    m_waylandDisplay.clear();
}

bool HyprlandIPC::isAvailable() {
    refreshInstance();
    return m_available;
}

QString HyprlandIPC::instanceSignature() { refreshInstance(); return m_instanceSignature; }
QString HyprlandIPC::waylandDisplay() { refreshInstance(); return m_waylandDisplay; }
qint64 HyprlandIPC::compositorPid() { refreshInstance(); return m_pid; }

QString HyprlandIPC::socketPath() {
    refreshInstance();
    return runtimeDir() + "/hypr/" + m_instanceSignature + "/.socket.sock";
}

QString HyprlandIPC::eventSocketPath() {
    refreshInstance();
    return runtimeDir() + "/hypr/" + m_instanceSignature + "/.socket2.sock";
}

QByteArray HyprlandIPC::request(const QString& command) {
    refreshInstance();
    if (!m_available) {
        return {};
    }

    QLocalSocket socket;
    socket.connectToServer(socketPath());
    if (!socket.waitForConnected(200)) {
        qWarning() << "[HyprlandIPC] connessione al socket fallita:" << socket.errorString();
        return {};
    }

    socket.write(command.toUtf8());
    socket.flush();
    socket.waitForBytesWritten(200);

    if (!socket.waitForReadyRead(300)) {
        return {};
    }

    QByteArray reply = socket.readAll();
    while (socket.waitForReadyRead(20)) {
        reply += socket.readAll();
    }
    return reply;
}

QJsonArray HyprlandIPC::requestJsonArray(const QString& command) {
    const QByteArray raw = request("j/" + command);
    const auto doc = QJsonDocument::fromJson(raw);
    return doc.isArray() ? doc.array() : QJsonArray{};
}

QJsonObject HyprlandIPC::requestJsonObject(const QString& command) {
    const QByteArray raw = request("j/" + command);
    const auto doc = QJsonDocument::fromJson(raw);
    return doc.isObject() ? doc.object() : QJsonObject{};
}

QVector<HyprWorkspace> HyprlandIPC::activeWorkspaces() {
    QVector<HyprWorkspace> result;
    const QJsonArray arr = requestJsonArray("workspaces");
    result.reserve(arr.size());

    for (const auto& val : arr) {
        const auto obj = val.toObject();
        HyprWorkspace ws;
        ws.id = obj.value("id").toInt();
        ws.name = obj.value("name").toString();
        ws.monitor = obj.value("monitor").toString();
        ws.windowCount = obj.value("windows").toInt();
        ws.hasFullscreen = obj.value("hasfullscreen").toBool();
        result.push_back(ws);
    }
    return result;
}

QVector<HyprWindow> HyprlandIPC::clients() {
    QVector<HyprWindow> result;
    const QJsonArray arr = requestJsonArray("clients");
    result.reserve(arr.size());

    for (const auto& val : arr) {
        const auto obj = val.toObject();
        HyprWindow win;
        win.address = obj.value("address").toString();
        win.cls = obj.value("class").toString();
        win.title = obj.value("title").toString();
        win.workspaceId = obj.value("workspace").toObject().value("id").toInt();
        win.floating = obj.value("floating").toBool();
        win.fullscreen = obj.value("fullscreen").toBool();
        result.push_back(win);
    }
    return result;
}

std::optional<HyprWorkspace> HyprlandIPC::activeWorkspace() {
    const auto obj = requestJsonObject("activeworkspace");
    if (obj.isEmpty()) {
        return std::nullopt;
    }
    HyprWorkspace ws;
    ws.id = obj.value("id").toInt();
    ws.name = obj.value("name").toString();
    ws.monitor = obj.value("monitor").toString();
    ws.windowCount = obj.value("windows").toInt();
    ws.hasFullscreen = obj.value("hasfullscreen").toBool();
    return ws;
}

std::optional<HyprWindow> HyprlandIPC::activeWindow() {
    const auto obj = requestJsonObject("activewindow");
    if (obj.isEmpty() || !obj.contains("address")) {
        return std::nullopt;
    }
    HyprWindow win;
    win.address = obj.value("address").toString();
    win.cls = obj.value("class").toString();
    win.title = obj.value("title").toString();
    win.workspaceId = obj.value("workspace").toObject().value("id").toInt();
    win.floating = obj.value("floating").toBool();
    win.fullscreen = obj.value("fullscreen").toBool();
    return win;
}

bool HyprlandIPC::dispatch(const QString& dispatcher, const QString& args) {
    refreshInstance();
    if (!m_available) {
        return false;
    }
    QString cmd = "dispatch " + dispatcher;
    if (!args.isEmpty()) {
        cmd += " " + args;
    }
    const QByteArray reply = request(cmd);
    return reply.trimmed() == "ok";
}

bool HyprlandIPC::switchToWorkspace(int id) {
    return dispatch("workspace", QString::number(id));
}

bool HyprlandIPC::moveActiveWindowToWorkspace(int id) {
    return dispatch("movetoworkspace", QString::number(id));
}

bool HyprlandIPC::moveWindowToWorkspace(const QString& windowAddress, int id) {
    return dispatch("movetoworkspacesilent",
                     QString("%1,address:%2").arg(id).arg(windowAddress));
}

} // namespace nicoos
