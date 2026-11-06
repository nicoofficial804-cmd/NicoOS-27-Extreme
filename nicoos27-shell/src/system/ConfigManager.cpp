#include "nicoos/ConfigManager.hpp"

#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QStandardPaths>
#include <QTextStream>

namespace nicoos {

ConfigManager& ConfigManager::instance() {
    static ConfigManager inst;
    return inst;
}

ConfigManager::ConfigManager(QObject* parent) : QObject(parent) {
    applyDefaults();
    load();
}

QString ConfigManager::configFilePath() const {
    const QString base = QDir::homePath() + "/.config/nicoos";
    QDir().mkpath(base);
    return base + "/nicoos.conf";
}

void ConfigManager::applyDefaults() {
    m_accentColor = QColor(0x00, 0xd9, 0xff);
    m_panelOpacity = 0.72;
    m_panelPosition = PanelPosition::Top;
    m_animationSpeedMs = 180;
    m_clockFormat = "HH:mm";
    m_launcherShortcut = "SUPER,R";
    m_uiScale = 1.0;
    m_panelHeight = 34;
    m_workspaceWrapAround = true;
}

void ConfigManager::load() {
    const QString path = configFilePath();

    if (!QFile::exists(path)) {
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&f);
            out << "# NicoOS 27 Extreme shell configuration\n"
                << "[appearance]\n"
                << "accent_color=#00d9ff\n"
                << "panel_opacity=0.72\n"
                << "ui_scale=1.0\n\n"
                << "[panel]\n"
                << "position=top\n"
                << "height=34\n\n"
                << "[behavior]\n"
                << "animation_speed_ms=180\n"
                << "clock_format=HH:mm\n"
                << "launcher_shortcut=SUPER,R\n"
                << "workspace_wrap_around=true\n";
        }
    }

    m_settings = std::make_unique<QSettings>(path, QSettings::IniFormat);

    const QString accent = m_settings->value("appearance/accent_color", "#00d9ff").toString();
    m_accentColor = QColor(accent);
    if (!m_accentColor.isValid()) {
        m_accentColor = QColor(0x00, 0xd9, 0xff);
    }

    m_panelOpacity = m_settings->value("appearance/panel_opacity", 0.72).toDouble();
    m_uiScale = m_settings->value("appearance/ui_scale", 1.0).toDouble();

    const QString pos = m_settings->value("panel/position", "top").toString();
    m_panelPosition = (pos.compare("bottom", Qt::CaseInsensitive) == 0)
                           ? PanelPosition::Bottom
                           : PanelPosition::Top;
    m_panelHeight = m_settings->value("panel/height", 34).toInt();

    m_animationSpeedMs = m_settings->value("behavior/animation_speed_ms", 180).toInt();
    m_clockFormat = m_settings->value("behavior/clock_format", "HH:mm").toString();
    m_launcherShortcut = m_settings->value("behavior/launcher_shortcut", "SUPER,R").toString();
    m_workspaceWrapAround = m_settings->value("behavior/workspace_wrap_around", true).toBool();

    m_iconTheme = m_settings->value("appearance/icon_theme").toString();
    m_wallpaperPath = m_settings->value("wallpaper/path").toString();
    m_wallpaperDirs = m_settings->value("wallpaper/dirs").toStringList();

    m_lockConfigPath = m_settings->value("lock/hyprlock_config").toString().trimmed();
    m_lockFont = m_settings->value("lock/font", "Noto Sans").toString().trimmed();
    if (m_lockFont.isEmpty()) m_lockFont = "Noto Sans";
    m_lockSoftwareRender = m_settings->value("lock/software_render", "auto").toString().trimmed().toLower();

    emit configReloaded();
}

void ConfigManager::setWallpaperPath(const QString& path) {
    m_wallpaperPath = path;
    if (m_settings) {
        m_settings->setValue("wallpaper/path", path);
        m_settings->sync();
    }
}

void ConfigManager::watchForChanges() {
    auto* watcher = new QFileSystemWatcher(this);
    watcher->addPath(configFilePath());
    connect(watcher, &QFileSystemWatcher::fileChanged, this, [this, watcher](const QString& path) {
        load();
        // Alcuni editor sostituiscono il file invece di scriverlo sul posto,
        // facendolo uscire dalla lista di osservazione; lo si riaggiunge
        // per sicurezza.
        if (!watcher->files().contains(path)) {
            watcher->addPath(path);
        }
    });
}

} // namespace nicoos
