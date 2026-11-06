#include "nicoos/TaskbarModel.hpp"
#include "nicoos/AppIconProvider.hpp"
#include "nicoos/HyprlandEventListener.hpp"
#include "nicoos/HyprlandIPC.hpp"

#include <QFileInfo>
#include <algorithm>

namespace nicoos {

TaskbarModel::TaskbarModel(HyprlandEventListener* events, QObject* parent)
    : QAbstractListModel(parent) {
    m_indexer = std::make_unique<AppIndexer>();
    buildIconLookup();

    m_debounce.setSingleShot(true);
    m_debounce.setInterval(40);
    connect(&m_debounce, &QTimer::timeout, this, &TaskbarModel::refresh);

    auto schedule = [this] { m_debounce.start(); };
    connect(events, &HyprlandEventListener::workspaceChanged, this, schedule);
    connect(events, &HyprlandEventListener::activeWindowChanged, this, schedule);
    connect(events, &HyprlandEventListener::windowOpened, this, schedule);
    connect(events, &HyprlandEventListener::windowClosed, this, schedule);
    connect(events, &HyprlandEventListener::windowMoved, this, schedule);
    connect(events, &HyprlandEventListener::rawEvent, this,
            [this](const QString& name, const QString&) {
                // cambi di titolo, toggle fullscreen, ecc.
                if (name.startsWith("windowtitle") || name.startsWith("activewindowv2")) {
                    m_debounce.start();
                }
            });

    refresh();
}

TaskbarModel::~TaskbarModel() = default;

int TaskbarModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant TaskbarModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) return {};
    const auto& e = m_entries[index.row()];
    switch (role) {
        case AddressRole:   return e.address;
        case ClassRole:     return e.windowClass;
        case TitleRole:     return e.title;
        case IconKeyRole:   return e.iconKey;
        case WorkspaceRole: return e.workspaceId;
        case ActiveRole:    return e.active;
        case MinimizedRole: return e.minimized;
        default:            return {};
    }
}

QHash<int, QByteArray> TaskbarModel::roleNames() const {
    return {
        {AddressRole, "winAddress"},
        {ClassRole, "winClass"},
        {TitleRole, "winTitle"},
        {IconKeyRole, "winIconKey"},
        {WorkspaceRole, "winWorkspace"},
        {ActiveRole, "winActive"},
        {MinimizedRole, "winMinimized"},
    };
}

void TaskbarModel::buildIconLookup() {
    auto add = [this](const QString& key, const DesktopApp& app) {
        const QString k = key.trimmed().toLower();
        if (!k.isEmpty() && !m_lookup.contains(k)) m_lookup.insert(k, app);
    };
    for (const auto& app : m_indexer->apps()) {
        add(app.id, app);
        add(app.wmClass, app);
        const QString firstToken = app.execLine.split(' ', Qt::SkipEmptyParts).value(0);
        add(QFileInfo(firstToken).fileName(), app);
        add(app.name, app);
    }
}

QString TaskbarModel::iconKeyFor(const QString& windowClass) const {
    const QString key = windowClass.toLower();
    auto it = m_lookup.constFind(key);
    if (it == m_lookup.constEnd()) {
        // Classi reverse-DNS ("org.mozilla.firefox"): prova l'ultimo segmento.
        it = m_lookup.constFind(key.section('.', -1));
    }
    if (it != m_lookup.constEnd()) {
        return AppIconProvider::makeId(it->iconName, it->name);
    }
    return AppIconProvider::makeId(key, windowClass);
}

int TaskbarModel::indexOfAddress(const QString& address) const {
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].address == address) return i;
    }
    return -1;
}

void TaskbarModel::refresh() {
    auto& ipc = HyprlandIPC::instance();
    const auto clients = ipc.clients();
    const auto activeWin = ipc.activeWindow();
    const QString activeAddr = activeWin ? activeWin->address : QString();

    QVector<TaskbarEntry> fresh;
    fresh.reserve(clients.size());
    for (const auto& c : clients) {
        if (c.cls.isEmpty() && c.title.isEmpty()) continue;  // non mappata
        if (c.cls == "nicoos-shell") continue;               // nostre finestre di fallback
        TaskbarEntry e;
        e.address = c.address;
        e.windowClass = c.cls;
        e.title = c.title;
        e.workspaceId = c.workspaceId;
        e.active = (c.address == activeAddr);
        e.minimized = c.workspaceId < 0;  // i workspace speciali hanno id negativi
        e.iconKey = iconKeyFor(c.cls);
        fresh.push_back(e);
    }

    auto findFresh = [&fresh](const QString& address) {
        return std::find_if(fresh.begin(), fresh.end(),
                            [&address](const TaskbarEntry& f) { return f.address == address; });
    };

    const int before = m_entries.size();

    // 1) rimuove le finestre sparite
    for (int i = m_entries.size() - 1; i >= 0; --i) {
        if (findFresh(m_entries[i].address) == fresh.end()) {
            beginRemoveRows(QModelIndex(), i, i);
            m_entries.removeAt(i);
            endRemoveRows();
        }
    }

    // 2) aggiorna le finestre cambiate (titolo, focus, minimizzata, ...)
    for (int i = 0; i < m_entries.size(); ++i) {
        const auto it = findFresh(m_entries[i].address);
        if (it != fresh.end() && !(m_entries[i] == *it)) {
            m_entries[i] = *it;
            emit dataChanged(index(i), index(i));
        }
    }

    // 3) aggiunge le nuove finestre in coda (mantiene l'ordine di
    //    apertura, come Windows)
    for (const auto& f : fresh) {
        if (indexOfAddress(f.address) < 0) {
            beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size());
            m_entries.push_back(f);
            endInsertRows();
        }
    }

    if (before != m_entries.size()) emit countChanged();
}

void TaskbarModel::activate(const QString& address) {
    const int row = indexOfAddress(address);
    if (row < 0) return;
    const TaskbarEntry entry = m_entries[row];
    auto& ipc = HyprlandIPC::instance();

    if (entry.minimized) {
        const auto ws = ipc.activeWorkspace();
        const int target = ws ? ws->id : 1;
        ipc.dispatch("movetoworkspacesilent", QString("%1,address:%2").arg(target).arg(address));
        ipc.dispatch("focuswindow", "address:" + address);
    } else if (entry.active) {
        ipc.dispatch("movetoworkspacesilent", "special:minimized,address:" + address);
    } else {
        ipc.dispatch("focuswindow", "address:" + address);
    }
}

void TaskbarModel::close(const QString& address) {
    HyprlandIPC::instance().dispatch("closewindow", "address:" + address);
}

} // namespace nicoos
