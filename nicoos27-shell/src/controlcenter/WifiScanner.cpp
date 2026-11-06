#include "nicoos/WifiScanner.hpp"
#include "nicoos/ProcessUtil.hpp"

#include <algorithm>

namespace nicoos {

namespace {
// nmcli -t separa i campi con ':' ed esegue l'escape di ':' e '\' letterali con
// '\'.
QStringList splitTerse(const QString& line) {
    QStringList out;
    QString cur;
    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line[i];
        if (c == '\\' && i + 1 < line.size()) {
            cur += line[++i];
        } else if (c == ':') {
            out << cur;
            cur.clear();
        } else {
            cur += c;
        }
    }
    out << cur;
    return out;
}
} // namespace

WifiScanner::WifiScanner(QObject* parent) : QAbstractListModel(parent) {
    m_available = hasExecutable("nmcli");
}

int WifiScanner::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_networks.size();
}

QVariant WifiScanner::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_networks.size()) return {};
    const auto& n = m_networks[index.row()];
    switch (role) {
        case SsidRole:   return n.ssid;
        case SignalRole: return n.signal;
        case SecureRole: return n.secure;
        case ActiveRole: return n.active;
        default:         return {};
    }
}

QHash<int, QByteArray> WifiScanner::roleNames() const {
    return {{SsidRole, "ssid"}, {SignalRole, "strength"}, {SecureRole, "secure"}, {ActiveRole, "active"}};
}

void WifiScanner::setBusy(bool busy) {
    if (m_busy == busy) return;
    m_busy = busy;
    emit busyChanged();
}

void WifiScanner::scan() {
    if (!m_available || m_busy) return;
    setBusy(true);
    runCapture(this, "nmcli",
               {"-t", "-f", "IN-USE,SSID,SIGNAL,SECURITY", "device", "wifi", "list"},
               [this](int code, const QString& out) {
                   setBusy(false);
                   if (code != 0) return;

                   QVector<Network> list;
                   const QStringList lines = out.split('\n', Qt::SkipEmptyParts);
                   for (const QString& line : lines) {
                       const QStringList f = splitTerse(line);
                       if (f.size() < 4 || f[1].isEmpty()) continue;
                       Network n;
                       n.active = (f[0] == "*");
                       n.ssid = f[1];
                       n.signal = f[2].toInt();
                       n.secure = !f[3].isEmpty() && f[3] != "--";

                       // Una riga per SSID: tieni il punto di accesso più
                       // forte.
                       auto it = std::find_if(list.begin(), list.end(),
                                              [&n](const Network& o) { return o.ssid == n.ssid; });
                       if (it == list.end()) {
                           list.push_back(n);
                       } else {
                           it->active = it->active || n.active;
                           it->signal = std::max(it->signal, n.signal);
                       }
                   }
                   std::sort(list.begin(), list.end(), [](const Network& a, const Network& b) {
                       if (a.active != b.active) return a.active;
                       return a.signal > b.signal;
                   });

                   beginResetModel();
                   m_networks = list;
                   endResetModel();
                   emit countChanged();
               });
}

bool WifiScanner::isSecure(const QString& ssid) const {
    for (const auto& n : m_networks) {
        if (n.ssid == ssid) return n.secure;
    }
    return false;
}

void WifiScanner::finishConnect(const QString& ssid, int exitCode, bool hadPassword) {
    setBusy(false);
    const bool ok = (exitCode == 0);
    if (!ok && !hadPassword && isSecure(ssid)) {
        emit passwordRequired(ssid);
    }
    emit connectFinished(ssid, ok);
    scan();
}

void WifiScanner::connectTo(const QString& ssid) {
    if (!m_available || m_busy) return;
    setBusy(true);
    runCapture(this, "nmcli", {"device", "wifi", "connect", ssid},
               [this, ssid](int code, const QString&) { finishConnect(ssid, code, false); });
}

void WifiScanner::connectWithPassword(const QString& ssid, const QString& password) {
    if (!m_available || m_busy) return;
    setBusy(true);
    // nmcli riceve la password come argomento (brevemente visibile nella lista
    // dei processi); accettabile per un desktop a singolo utente.
    runCapture(this, "nmcli", {"device", "wifi", "connect", ssid, "password", password},
               [this, ssid](int code, const QString&) { finishConnect(ssid, code, true); });
}

} // namespace nicoos
