#include "nicoos/WallpaperManager.hpp"
#include "nicoos/ConfigManager.hpp"
#include "nicoos/Notify.hpp"
#include "nicoos/ProcessUtil.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRandomGenerator>
#include <QSet>
#include <QStandardPaths>
#include <QUrl>
#include <algorithm>

namespace nicoos {

// ---------- WallpaperModel ----------

WallpaperModel::WallpaperModel(QObject* parent) : QAbstractListModel(parent) {}

int WallpaperModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_paths.size();
}

QVariant WallpaperModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_paths.size()) return {};
    const QString& path = m_paths[index.row()];
    switch (role) {
        case PathRole:    return path;
        case NameRole:    return QFileInfo(path).completeBaseName();
        case UrlRole:     return QUrl::fromLocalFile(path).toString();
        case CurrentRole: return path == m_current;
        default:          return {};
    }
}

QHash<int, QByteArray> WallpaperModel::roleNames() const {
    return {{PathRole, "wpPath"}, {NameRole, "wpName"}, {UrlRole, "wpUrl"}, {CurrentRole, "isCurrent"}};
}

void WallpaperModel::setPaths(const QStringList& paths) {
    beginResetModel();
    m_paths = paths;
    endResetModel();
}

void WallpaperModel::setCurrent(const QString& path) {
    if (m_current == path) return;
    m_current = path;
    if (!m_paths.isEmpty()) {
        emit dataChanged(index(0), index(m_paths.size() - 1), {CurrentRole});
    }
}

// ---------- WallpaperManager ----------

WallpaperManager::WallpaperManager(QObject* parent) : QObject(parent) {
    // Assicurati che esista la cartella "metti qui i tuoi sfondi".
    QDir().mkpath(QDir::homePath() + "/.config/nicoos/wallpapers");
    m_current = ConfigManager::instance().wallpaperPath();
    m_model.setCurrent(m_current);
    refresh();
}

bool WallpaperManager::available() const {
    return hasExecutable("swaybg") || hasExecutable("swww");
}

QStringList WallpaperManager::searchDirs() const {
    QStringList dirs = ConfigManager::instance().wallpaperDirs();
    const QString pics = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    dirs << pics + "/Wallpapers" << pics + "/wallpapers" << pics + "/Sfondi"
         << QDir::homePath() + "/.config/nicoos/wallpapers"
         << "/usr/share/backgrounds" << "/usr/share/nicoos/wallpapers";
    dirs.removeDuplicates();
    return dirs;
}

void WallpaperManager::refresh() {
    static const QSet<QString> kExt{"png", "jpg", "jpeg", "webp", "bmp"};
    QStringList found;
    for (const QString& dir : searchDirs()) {
        if (!QDir(dir).exists()) continue;
        QDirIterator it(dir, QDir::Files, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
        while (it.hasNext() && found.size() < 500) {
            const QString path = it.next();
            if (kExt.contains(QFileInfo(path).suffix().toLower())) found << path;
        }
    }
    found.removeDuplicates();
    std::sort(found.begin(), found.end(), [](const QString& a, const QString& b) {
        return QFileInfo(a).fileName().compare(QFileInfo(b).fileName(), Qt::CaseInsensitive) < 0;
    });
    m_model.setPaths(found);
}

void WallpaperManager::apply(const QString& path) {
    if (!QFile::exists(path)) return;

    if (hasExecutable("swaybg")) {
        // Avvia prima il nuovo swaybg e solo dopo ferma il vecchio, cosi'
        // lo schermo non lampeggia mai mostrando lo sfondo vuoto.
        const QString script = R"(old=$(pgrep -x swaybg | tr '\n' ' ')
swaybg -i "$1" -m fill >/dev/null 2>&1 &
sleep 0.6
[ -n "$old" ] && kill $old 2>/dev/null
wait)";
        QProcess::startDetached("sh", {"-c", script, "sh", path});
    } else if (hasExecutable("swww")) {
        const QString script = R"(swww query >/dev/null 2>&1 || { swww-daemon >/dev/null 2>&1 & sleep 1; }
swww img "$1" --transition-type grow --transition-fps 60)";
        QProcess::startDetached("sh", {"-c", script, "sh", path});
    } else {
        notifyUser("Sfondo non applicabile", "Installa swaybg (o swww): sudo pacman -S swaybg");
        return;
    }

    m_current = path;
    m_model.setCurrent(path);
    if (ConfigManager::instance().wallpaperPath() != path) {
        ConfigManager::instance().setWallpaperPath(path);
    }
    emit currentChanged();
}

void WallpaperManager::applyRandom() {
    QStringList candidates = m_model.paths();
    candidates.removeAll(m_current);
    if (candidates.isEmpty()) return;
    apply(candidates.at(int(QRandomGenerator::global()->bounded(candidates.size()))));
}

void WallpaperManager::restore() {
    if (!m_current.isEmpty() && QFile::exists(m_current)) {
        apply(m_current);
    }
}

} // namespace nicoos
