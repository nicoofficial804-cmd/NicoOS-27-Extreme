#include "nicoos/LauncherWindow.hpp"
#include "nicoos/FuzzyMatcher.hpp"
#include "nicoos/ConfigManager.hpp"
#include "nicoos/AppIconProvider.hpp"

#include <QQmlContext>
#include <QScreen>
#include <QGuiApplication>

namespace nicoos {

// ---------- LauncherResultsModel ----------

LauncherResultsModel::LauncherResultsModel(AppIndexer* indexer, QObject* parent)
    : QAbstractListModel(parent), m_indexer(indexer) {
    refilter();
    connect(indexer, &AppIndexer::indexUpdated, this, [this] { refilter(); });
}

int LauncherResultsModel::rowCount(const QModelIndex&) const {
    return m_filtered.size();
}

QVariant LauncherResultsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_filtered.size()) return {};
    const auto& app = m_filtered[index.row()].app;
    switch (role) {
        case NameRole: return app.name;
        case CommentRole: return app.comment;
        case IconRole: return AppIconProvider::makeId(app.iconName, app.name);
        case AppIdRole: return app.id;
        default: return {};
    }
}

QHash<int, QByteArray> LauncherResultsModel::roleNames() const {
    return {
        {NameRole, "name"},
        {CommentRole, "comment"},
        {IconRole, "iconName"},
        {AppIdRole, "appId"},
    };
}

void LauncherResultsModel::setQuery(const QString& q) {
    if (m_query == q) return;
    m_query = q;
    refilter();
    emit queryChanged();
}

void LauncherResultsModel::refilter() {
    beginResetModel();
    m_filtered = FuzzyMatcher::match(m_query, m_indexer->apps());
    // Limita i risultati mostrati: un launcher non deve mai disegnare
    // centinaia di righe, e una lista corta mantiene leggera la ListView.
    if (m_filtered.size() > 40) {
        m_filtered.resize(40);
    }
    endResetModel();
}

void LauncherResultsModel::launch(int row) {
    if (row < 0 || row >= m_filtered.size()) return;
    m_indexer->launch(m_filtered[row].app.id);
    emit appLaunched();
}

void LauncherResultsModel::launchTopResult() {
    if (!m_filtered.isEmpty()) {
        launch(0);
    }
}

// ---------- LauncherWindow ----------

LauncherWindow::LauncherWindow(QObject* parent) : QObject(parent) {
    m_indexer = std::make_unique<AppIndexer>();
    m_model = std::make_unique<LauncherResultsModel>(m_indexer.get());
    buildWindow();
}

LauncherWindow::~LauncherWindow() = default;

void LauncherWindow::buildWindow() {
    m_window = std::make_unique<LayerSurfaceWindow>(ShellLayer::Overlay);

    auto& cfg = ConfigManager::instance();
    auto* ctx = m_window->rootContext();
    ctx->setContextProperty("launcherModel", m_model.get());
    ctx->setContextProperty("accentColor", cfg.accentColor());

    m_window->setSource(QUrl("qrc:/nicoos/qml/Launcher.qml"));
    m_window->setKeyboardMode(KeyboardMode::Exclusive);
    m_window->setExclusivePopup(true);

    // Nessuna ancora: è il compositore a centrare la superficie sull'uscita
    // video.
    m_window->resize(QSize(560, 420));
    m_window->setVisible(false);

    connect(m_model.get(), &LauncherResultsModel::appLaunched, this, [this] {
        m_window->setVisible(false);
    });
}

bool LauncherWindow::isVisible() const {
    return m_window->isVisible();
}

void LauncherWindow::toggle() {
    if (m_window->isVisible()) {
        m_window->setVisible(false);
        return;
    }
    m_model->setQuery("");
    m_window->showAsLayerSurface();
    m_window->requestActivate();
}

} // namespace nicoos
