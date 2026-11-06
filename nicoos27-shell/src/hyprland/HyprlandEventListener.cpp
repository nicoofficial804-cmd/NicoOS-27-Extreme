#include "nicoos/HyprlandEventListener.hpp"
#include "nicoos/HyprlandIPC.hpp"

#include <QProcessEnvironment>
#include <QTimer>
#include <QDebug>

namespace nicoos {

HyprlandEventListener::HyprlandEventListener(QObject* parent) : QObject(parent) {
    connect(&m_socket, &QLocalSocket::readyRead, this, &HyprlandEventListener::onReadyRead);
    connect(&m_socket, &QLocalSocket::disconnected, this, &HyprlandEventListener::onDisconnected);
}

void HyprlandEventListener::start() {
    tryConnect();
}

void HyprlandEventListener::tryConnect() {
    if (m_socket.state() != QLocalSocket::UnconnectedState) return;
    if (!HyprlandIPC::instance().isAvailable()) {
        // Avviata prima che Hyprland fosse pronto, o tra due sessioni di
        // Hyprland.
        QTimer::singleShot(m_reconnectDelayMs, this, &HyprlandEventListener::tryConnect);
        m_reconnectDelayMs = std::min(m_reconnectDelayMs * 2, 15000);
        return;
    }
    m_socket.connectToServer(HyprlandIPC::instance().eventSocketPath());
    if (!m_socket.waitForConnected(300)) {
        // Il compositore potrebbe non essere del tutto su; aspetta e riprova.
        QTimer::singleShot(m_reconnectDelayMs, this, &HyprlandEventListener::tryConnect);
        m_reconnectDelayMs = std::min(m_reconnectDelayMs * 2, 15000);
    } else {
        m_reconnectDelayMs = 1000;
    }
}

void HyprlandEventListener::onDisconnected() {
    QTimer::singleShot(m_reconnectDelayMs, this, &HyprlandEventListener::tryConnect);
}

void HyprlandEventListener::onReadyRead() {
    m_buffer += m_socket.readAll();

    int newlineIdx;
    while ((newlineIdx = m_buffer.indexOf('\n')) != -1) {
        const QByteArray lineBytes = m_buffer.left(newlineIdx);
        m_buffer.remove(0, newlineIdx + 1);
        handleLine(QString::fromUtf8(lineBytes));
    }
}

void HyprlandEventListener::handleLine(const QString& line) {
    // Le righe evento di Hyprland sono del tipo: "NOMEEVENTO>>DATI"
    const int sep = line.indexOf(">>");
    if (sep == -1) {
        return;
    }

    const QString name = line.left(sep);
    const QString payload = line.mid(sep + 2);

    if (name == "workspace") {
        emit workspaceChanged(payload.toInt());
    } else if (name == "activewindow") {
        const auto parts = payload.split(',');
        const QString cls = parts.value(0);
        const QString title = parts.size() > 1 ? parts.mid(1).join(',') : QString();
        emit activeWindowChanged(cls, title);
    } else if (name == "openwindow") {
        const auto parts = payload.split(',');
        if (parts.size() >= 2) {
            emit windowOpened(parts[0], parts[2 % parts.size()]);
        }
    } else if (name == "closewindow") {
        emit windowClosed(payload);
    } else if (name == "movewindow") {
        const auto parts = payload.split(',');
        if (parts.size() >= 2) {
            emit windowMoved(parts[0], parts[1].toInt());
        }
    } else if (name == "focusedmon") {
        emit monitorChanged(payload.split(',').value(0));
    } else if (name == "configreloaded") {
        emit configReloaded();
    }

    emit rawEvent(name, payload);
}

} // namespace nicoos
