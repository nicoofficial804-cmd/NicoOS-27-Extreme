#pragma once

#include <QObject>
#include <QString>
#include <QColor>
#include <QStringList>
#include <QSettings>
#include <memory>

namespace nicoos {

enum class PanelPosition { Top, Bottom };

// Carica e sorveglia ~/.config/nicoos/nicoos.conf.
// Tutti i moduli della shell leggono da qui le impostazioni visive e di
// comportamento, così l'utente non deve mai ricompilare per cambiare stile o
// riconfigurare la shell.
class ConfigManager : public QObject {
    Q_OBJECT

public:
    static ConfigManager& instance();

    void load();
    void watchForChanges();

    QColor accentColor() const { return m_accentColor; }
    double panelOpacity() const { return m_panelOpacity; }
    PanelPosition panelPosition() const { return m_panelPosition; }
    int animationSpeedMs() const { return m_animationSpeedMs; }
    QString clockFormat() const { return m_clockFormat; }
    QString launcherShortcut() const { return m_launcherShortcut; }
    double uiScale() const { return m_uiScale; }
    int panelHeight() const { return m_panelHeight; }
    bool workspaceWrapAround() const { return m_workspaceWrapAround; }
    // [behavior] workspace_count: quante aree di lavoro la shell tiene sempre
    // presenti (1..N), anche quando sono vuote. Sono aree reali di Hyprland:
    // vengono create al primo uso.
    int workspaceCount() const { return m_workspaceCount; }
    QString iconTheme() const { return m_iconTheme; }
    QString wallpaperPath() const { return m_wallpaperPath; }
    QStringList wallpaperDirs() const { return m_wallpaperDirs; }
    // [lock] hyprlock_config: percorso di una configurazione hyprlock scritta
    // a mano. Se vuoto la shell genera e usa ~/.config/nicoos/hyprlock.conf.
    QString lockConfigPath() const { return m_lockConfigPath; }
    QString lockFont() const { return m_lockFont; }
    // [lock] software_render = auto | yes | no  (auto: GL software nelle VM)
    QString lockSoftwareRender() const { return m_lockSoftwareRender; }
    // Salva in modo permanente lo sfondo scelto in
    // ~/.config/nicoos/nicoos.conf.
    void setWallpaperPath(const QString& path);

signals:
    void configReloaded();

private:
    explicit ConfigManager(QObject* parent = nullptr);
    QString configFilePath() const;
    void applyDefaults();

    QColor m_accentColor{0x00, 0xd9, 0xff};
    double m_panelOpacity{0.72};
    PanelPosition m_panelPosition{PanelPosition::Top};
    int m_animationSpeedMs{180};
    QString m_clockFormat{"HH:mm"};
    QString m_launcherShortcut{"SUPER,R"};
    double m_uiScale{1.0};
    int m_panelHeight{34};
    bool m_workspaceWrapAround{true};
    int m_workspaceCount{10};
    QString m_iconTheme;
    QString m_wallpaperPath;
    QStringList m_wallpaperDirs;
    QString m_lockConfigPath;
    QString m_lockFont{"Noto Sans"};
    QString m_lockSoftwareRender{"auto"};

    std::unique_ptr<QSettings> m_settings;
};

} // namespace nicoos
