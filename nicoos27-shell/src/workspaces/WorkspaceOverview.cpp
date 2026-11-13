#include "nicoos/WorkspaceOverview.hpp"
#include "nicoos/HyprlandIPC.hpp"
#include "nicoos/HyprlandEventListener.hpp"
#include "nicoos/ConfigManager.hpp"
#include "nicoos/WindowPreviewCapture.hpp"

#include <QQmlContext>
#include <QQmlEngine>
#include <QScreen>
#include <QGuiApplication>
#include <QBuffer>
#include <QSet>
#include <algorithm>

namespace nicoos {

// ---------- WorkspaceListModel ----------

WorkspaceListModel::WorkspaceListModel(QObject* parent) : QAbstractListModel(parent) {
    refresh();
}

int WorkspaceListModel::rowCount(const QModelIndex&) const {
    return m_rows.size();
}

QVariant WorkspaceListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_rows.size()) return {};
    const auto& r = m_rows[index.row()];
    switch (role) {
        case IdRole: return r.id;
        case NameRole: return r.name;
        case WindowCountRole: return r.windowCount;
        case ActiveRole: return r.active;
        case FullscreenRole: return r.hasFullscreen;
        default: return {};
    }
}

QHash<int, QByteArray> WorkspaceListModel::roleNames() const {
    return {
        {IdRole, "wsId"},
        {NameRole, "wsName"},
        {WindowCountRole, "windowCount"},
        {ActiveRole, "isActive"},
        {FullscreenRole, "hasFullscreen"},
    };
}

void WorkspaceListModel::refresh() {
    // Costruisci prima la lista aggiornata, poi applicala come insieme
    // minimo di operazioni remove / insert / dataChanged. Un reset
    // completo del modello distruggerebbe ogni delegate e con esso le
    // animazioni QML in corso (i puntini dei workspace sfarfallerebbero
    // invece di scorrere).
    const auto workspaces = HyprlandIPC::instance().activeWorkspaces();
    const auto active = HyprlandIPC::instance().activeWorkspace();

    QVector<WorkspaceRow> fresh;
    fresh.reserve(workspaces.size() + ConfigManager::instance().workspaceCount());
    QSet<int> seen;
    for (const auto& ws : workspaces) {
        // I workspace speciali (scratchpad, il nostro parcheggio per le
        // finestre "minimizzate") hanno id negativi e non sono qualcosa
        // su cui l'utente passa.
        if (ws.id <= 0) continue;
        seen.insert(ws.id);
        WorkspaceRow row;
        row.id = ws.id;
        row.name = ws.name.isEmpty() ? QString::number(ws.id) : ws.name;
        row.windowCount = ws.windowCount;
        row.active = active && active->id == ws.id;
        row.hasFullscreen = ws.hasFullscreen;
        fresh.push_back(row);
    }

    // Le aree di lavoro 1..N sono aree *reali* della sessione, non segnaposto:
    // Hyprland distrugge un'area vuota appena la si lascia, quindi fermarsi
    // alla sua lista significherebbe mostrare un solo puntino e non poter
    // trascinare nulla su un'area nuova. La shell tiene quindi sempre
    // presenti le aree configurate ([behavior] workspace_count) e le crea in
    // Hyprland al primo uso — cambio area, trascinamento o clic sul puntino
    // passano da hyprctl dispatch, che crea davvero l'area mancante.
    const int configured = ConfigManager::instance().workspaceCount();
    for (int id = 1; id <= configured; ++id) {
        if (seen.contains(id)) continue;
        WorkspaceRow row;
        row.id = id;
        row.name = QString::number(id);
        row.active = active && active->id == id;
        fresh.push_back(row);
    }

    std::sort(fresh.begin(), fresh.end(), [](const WorkspaceRow& a, const WorkspaceRow& b) {
        return a.id < b.id;
    });

    // 1) elimina le righe che non esistono piu'
    for (int i = m_rows.size() - 1; i >= 0; --i) {
        const bool stillThere = std::any_of(fresh.begin(), fresh.end(),
            [&](const WorkspaceRow& f) { return f.id == m_rows[i].id; });
        if (!stillThere) {
            beginRemoveRows(QModelIndex(), i, i);
            m_rows.removeAt(i);
            endRemoveRows();
        }
    }

    // 2) inserisce le nuove righe nella posizione ordinata e aggiorna
    //    quelle cambiate
    for (int i = 0; i < fresh.size(); ++i) {
        if (i < m_rows.size() && m_rows[i].id == fresh[i].id) {
            const auto& o = m_rows[i];
            const auto& f = fresh[i];
            if (o.name != f.name || o.windowCount != f.windowCount ||
                o.active != f.active || o.hasFullscreen != f.hasFullscreen) {
                m_rows[i] = f;
                emit dataChanged(index(i), index(i));
            }
        } else {
            beginInsertRows(QModelIndex(), i, i);
            m_rows.insert(i, fresh[i]);
            endInsertRows();
        }
    }
}

void WorkspaceListModel::activate(int workspaceId) {
    HyprlandIPC::instance().switchToWorkspace(workspaceId);
}

void WorkspaceListModel::activateRelative(int delta) {
    if (m_rows.isEmpty() || delta == 0) return;
    int current = 0;
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].active) { current = i; break; }
    }
    int target = current + (delta > 0 ? 1 : -1);
    if (target < 0 || target >= m_rows.size()) {
        if (!ConfigManager::instance().workspaceWrapAround()) return;
        target = (target + m_rows.size()) % m_rows.size();
    }
    HyprlandIPC::instance().switchToWorkspace(m_rows[target].id);
}

int WorkspaceListModel::workspaceIdAt(int row) const {
    if (row < 0 || row >= m_rows.size()) return -1;
    return m_rows[row].id;
}

// ---------- WindowListModel ----------

WindowListModel::WindowListModel(WindowPreviewCapture* previews, QObject* parent)
    : QAbstractListModel(parent), m_previews(previews) {
    refresh();
}

int WindowListModel::rowCount(const QModelIndex&) const {
    return m_filtered.size();
}

QVariant WindowListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_filtered.size()) return {};
    const auto& w = m_filtered[index.row()];
    switch (role) {
        case AddressRole: return w.address;
        case ClassRole: return w.windowClass;
        case TitleRole: return w.title;
        case WorkspaceIdRole: return w.workspaceId;
        case FloatingRole: return w.floating;
        case PreviewSourceRole:
            // Contiene quanto basta perche' la source dell'Image in
            // Preview.qml possa farci da chiave, seguendo lo schema
            // "image://nicoosicon/<id>/<name>" usato dal provider di
            // icone del launcher, con la differenza che qui il provider
            // e' la cache di WindowPreviewCapture e non AppIconProvider —
            // vedi Preview.qml per come viene consumato (richiede tramite
            // requestPreview() direttamente invece che con un URL
            // image://, dato che le anteprime arrivano in modo asincrono
            // via segnale Qt e non con una richiesta pixmap sincrona a
            // QQuickImageProvider).
            return w.address;
        default: return {};
    }
}

QHash<int, QByteArray> WindowListModel::roleNames() const {
    return {
        {AddressRole, "winAddress"},
        {ClassRole, "winClass"},
        {TitleRole, "winTitle"},
        {WorkspaceIdRole, "winWorkspaceId"},
        {FloatingRole, "winFloating"},
        {PreviewSourceRole, "previewKey"},
    };
}

QVector<WindowRow> WindowListModel::allRows() const {
    QVector<WindowRow> rows;
    const auto clients = HyprlandIPC::instance().clients();
    rows.reserve(clients.size());
    for (const auto& c : clients) {
        WindowRow row;
        row.address = c.address;
        row.windowClass = c.cls;
        row.title = c.title;
        row.workspaceId = c.workspaceId;
        row.floating = c.floating;
        row.fullscreen = c.fullscreen;
        rows.push_back(row);
    }
    return rows;
}

void WindowListModel::refresh() {
    m_all = allRows();
    applyFilter();
}

void WindowListModel::setFilterText(const QString& text) {
    if (m_filterText == text) return;
    m_filterText = text;
    applyFilter();
    emit filterChanged();
}

void WindowListModel::applyFilter() {
    beginResetModel();

    if (m_filterText.trimmed().isEmpty()) {
        m_filtered = m_all;
    } else {
        const QString needle = m_filterText.trimmed().toLower();
        m_filtered.clear();
        for (const auto& w : m_all) {
            if (w.title.toLower().contains(needle) || w.windowClass.toLower().contains(needle)) {
                m_filtered.push_back(w);
            }
        }
    }

    endResetModel();
}

QVariantList WindowListModel::windowsForWorkspace(int workspaceId) const {
    QVariantList result;
    // Legge da m_all (non filtrato) invece che da m_filtered, cosi' la
    // griglia interna di ogni scheda workspace non risente del campo di
    // ricerca globale — la ricerca restringe altrove la vista piatta
    // "tutte le finestre", non cio' che mostra ogni scheda.
    for (const auto& w : m_all) {
        if (w.workspaceId != workspaceId) continue;
        QVariantMap row;
        row["winAddress"] = w.address;
        row["winClass"] = w.windowClass;
        row["winTitle"] = w.title;
        row["winWorkspaceId"] = w.workspaceId;
        result.push_back(row);
    }
    return result;
}

void WindowListModel::moveWindowToWorkspace(const QString& address, int workspaceId) {
    HyprlandIPC::instance().moveWindowToWorkspace(address, workspaceId);
    // Il chiamante (Extreme.qml) aggiorna entrambi i modelli quando
    // l'evento movewindow di Hyprland torna indietro tramite
    // HyprlandEventListener, quindi qui non serve un refresh() sincrono.
}

void WindowListModel::focusWindow(const QString& address) {
    HyprlandIPC::instance().dispatch("focuswindow", "address:" + address);
}

void WindowListModel::closeWindow(const QString& address) {
    HyprlandIPC::instance().dispatch("closewindow", "address:" + address);
}

// ---------- WorkspaceOverview ----------

WorkspaceOverview::WorkspaceOverview(HyprlandEventListener* events, QObject* parent)
    : QObject(parent), m_events(events) {
    m_previews = std::make_unique<WindowPreviewCapture>();
    m_workspaceModel = std::make_unique<WorkspaceListModel>();
    m_windowModel = std::make_unique<WindowListModel>(m_previews.get());
    buildWindow();

    auto refreshBoth = [this] { refreshAll(); };
    connect(events, &HyprlandEventListener::workspaceChanged, this, refreshBoth);
    connect(events, &HyprlandEventListener::windowOpened, this, refreshBoth);
    connect(events, &HyprlandEventListener::windowClosed, this, refreshBoth);
    connect(events, &HyprlandEventListener::windowMoved, this, refreshBoth);
}

WorkspaceOverview::~WorkspaceOverview() = default;

void WorkspaceOverview::refreshAll() {
    // La panoramica e' un overlay nascosto per la maggior parte del tempo:
    // non sprecare scambi IPC bloccanti per una superficie che nessuno
    // guarda. toggle() aggiorna esplicitamente subito prima di mostrarla.
    if (!m_window->isVisible()) return;
    m_workspaceModel->refresh();
    m_windowModel->refresh();
}

void WorkspaceOverview::buildWindow() {
    m_window = std::make_unique<LayerSurfaceWindow>(ShellLayer::Overlay);

    auto& cfg = ConfigManager::instance();
    auto* ctx = m_window->rootContext();
    ctx->setContextProperty("workspaceModel", m_workspaceModel.get());
    ctx->setContextProperty("windowModel", m_windowModel.get());
    ctx->setContextProperty("previewCapture", m_previews.get());
    ctx->setContextProperty("accentColor", cfg.accentColor());
    ctx->setContextProperty("animSpeedMs", cfg.animationSpeedMs());

    // Un oggetto adattatore separato, non-QObject — NON m_previews stesso
    // (vedi WindowPreviewCapture.hpp per il motivo per cui QObject e
    // QQuickImageProvider non possono convivere nella stessa classe).
    // L'engine prende possesso dell'adattatore e lo distrugge quando
    // viene distrutto; m_previews resta di proprieta' di questa classe
    // come al solito.
    m_window->rootContext()->engine()->addImageProvider(
        "windowpreview", new WindowPreviewImageProvider(m_previews.get()));

    m_window->setSource(QUrl("qrc:/nicoos/qml/WorkspaceOverview.qml"));
    m_window->setKeyboardMode(KeyboardMode::Exclusive);
    m_window->setExclusivePopup(true);

    // La modalita' "Extreme" vuole spazio vero sullo schermo — questa e'
    // la superficie di multitasking principale, quindi occupa la gran
    // parte del display invece di essere un modesto pannello centrato.
    if (auto* screen = QGuiApplication::primaryScreen()) {
        const auto geo = screen->geometry();
        m_window->resize(QSize(int(geo.width() * 0.94), int(geo.height() * 0.9)));
    }
    m_window->setVisible(false);
}

void WorkspaceOverview::toggle() {
    if (m_window->isVisible()) {
        m_window->setVisible(false);
        return;
    }
    m_workspaceModel->refresh();
    m_windowModel->refresh();
    m_window->showAsLayerSurface();
    m_window->requestActivate();
}

} // namespace nicoos
