#pragma once

#include <QObject>
#include <QVector>
#include <QString>
#include <optional>

namespace nicoos {

struct DesktopApp {
    QString id;          // nome base del file desktop, es. "org.kde.dolphin"
    QString name;
    QString comment;
    QString iconName;
    QString execLine;
    QString wmClass;     // StartupWMClass, usata per abbinare le finestre aperte
    bool terminal{false};
};

// Scansiona le directory applicazioni XDG standard alla ricerca di voci
// .desktop e analizza il sottoinsieme della Desktop Entry Specification che
// serve al launcher (Name, Comment, Icon, Exec, Terminal, NoDisplay, Hidden).
// L'avvio vero e proprio passa da QProcess + la $SHELL dell'utente /
// xdg-terminal-exec, cioè il normale meccanismo dei desktop Linux: questa
// classe costruisce soltanto l'indice ricercabile.
class AppIndexer : public QObject {
    Q_OBJECT

public:
    explicit AppIndexer(QObject* parent = nullptr);

    // Riscansione completa e sincrona di tutte le directory applicazioni.
    void reindex();

    const QVector<DesktopApp>& apps() const { return m_apps; }

    // Avvia l'app per id usando il normale meccanismo Exec di freedesktop
    // (codici di campo rimossi, app da terminale incapsulate come serve).
    bool launch(const QString& appId) const;

signals:
    void indexUpdated();

private:
    QStringList applicationDirectories() const;
    std::optional<DesktopApp> parseDesktopFile(const QString& path) const;

    QVector<DesktopApp> m_apps;
};

} // namespace nicoos
