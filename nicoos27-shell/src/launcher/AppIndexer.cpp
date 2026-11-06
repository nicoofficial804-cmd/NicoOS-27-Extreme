#include "nicoos/AppIndexer.hpp"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QRegularExpression>
#include <optional>

namespace nicoos {

AppIndexer::AppIndexer(QObject* parent) : QObject(parent) {
    reindex();
}

QStringList AppIndexer::applicationDirectories() const {
    QStringList dirs;
    const auto env = QProcessEnvironment::systemEnvironment();

    const QString dataHome = env.value("XDG_DATA_HOME",
        QDir::homePath() + "/.local/share");
    dirs << dataHome + "/applications";

    QString dataDirs = env.value("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
    for (const QString& d : dataDirs.split(':', Qt::SkipEmptyParts)) {
        dirs << d + "/applications";
    }
    return dirs;
}

std::optional<DesktopApp> AppIndexer::parseDesktopFile(const QString& path) const {
    // I file delle voci desktop sono simili a INI. QSettings divide i valori
    // sulle virgole e restituisce una QStringList, quindi ogni lettura passa
    // da str(), che ricompone i pezzi invece di perdere il testo dopo la prima
    // virgola.
    QSettings ini(path, QSettings::IniFormat);
    ini.beginGroup("Desktop Entry");

    auto str = [&ini](const QString& key) {
        return ini.value(key).toStringList().join(",");
    };

    const QString type = str("Type");
    if (!type.isEmpty() && type != "Application") return std::nullopt;
    if (ini.value("NoDisplay", false).toBool()) return std::nullopt;
    if (ini.value("Hidden", false).toBool()) return std::nullopt;

    const QString exec = str("Exec");
    if (exec.isEmpty()) return std::nullopt;

    DesktopApp app;
    app.id = QFileInfo(path).completeBaseName();
    app.name = str("Name");
    app.comment = str("Comment");
    app.iconName = str("Icon");
    app.execLine = exec;
    app.wmClass = str("StartupWMClass");
    app.terminal = ini.value("Terminal", false).toBool();

    if (app.name.isEmpty()) return std::nullopt;
    return app;
}

void AppIndexer::reindex() {
    m_apps.clear();
    QSet<QString> seenIds;

    for (const QString& dirPath : applicationDirectories()) {
        QDir dir(dirPath);
        if (!dir.exists()) continue;

        const auto entries = dir.entryList(QStringList{"*.desktop"}, QDir::Files);
        for (const QString& fileName : entries) {
            const QString fullPath = dir.filePath(fileName);
            auto parsed = parseDesktopFile(fullPath);
            if (!parsed) continue;

            // Le directory che vengono prima nell'ordine di
            // XDG_DATA_HOME/XDG_DATA_DIRS hanno la precedenza, come nella
            // ricerca standard dei file desktop.
            if (seenIds.contains(parsed->id)) continue;
            seenIds.insert(parsed->id);
            m_apps.push_back(*parsed);
        }
    }

    emit indexUpdated();
}

bool AppIndexer::launch(const QString& appId) const {
    const auto it = std::find_if(m_apps.begin(), m_apps.end(),
        [&](const DesktopApp& a) { return a.id == appId; });
    if (it == m_apps.end()) return false;

    // Rimuove i codici di campo freedesktop (%f %F %u %U %i %c %k ecc.):
    // il launcher non passa mai file/URI all'app invocata.
    QString cmd = it->execLine;
    cmd.remove(QRegularExpression("%[a-zA-Z]"));
    cmd = cmd.trimmed();

    if (it->terminal) {
        const QString term = qEnvironmentVariable("TERMINAL", "xterm");
        return QProcess::startDetached(term, {"-e", "sh", "-c", cmd});
    }

    return QProcess::startDetached("sh", {"-c", cmd});
}

} // namespace nicoos
