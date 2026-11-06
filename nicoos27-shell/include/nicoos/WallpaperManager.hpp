#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QStringList>
#include <QVector>

namespace nicoos {

// Elenco delle immagini di sfondo per il selettore.
class WallpaperModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role { PathRole = Qt::UserRole + 1, NameRole, UrlRole, CurrentRole };

    explicit WallpaperModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setPaths(const QStringList& paths);
    void setCurrent(const QString& path);
    QStringList paths() const { return m_paths; }

private:
    QStringList m_paths;
    QString m_current;
};

// Trova gli sfondi, ne applica uno, lo ricorda e lo ripristina all'avvio
// della shell. Backend: swaybg (predefinito) o swww.
//
// Cartelle cercate: <Immagini>/Wallpapers, <Immagini>/Sfondi,
// ~/.config/nicoos/wallpapers, /usr/share/backgrounds, più quelle elencate
// in nicoos.conf sotto [wallpaper] dirs=/a,/b.
class WallpaperManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString current READ current NOTIFY currentChanged)
    Q_PROPERTY(bool available READ available CONSTANT)

public:
    explicit WallpaperManager(QObject* parent = nullptr);

    WallpaperModel* model() { return &m_model; }
    QString current() const { return m_current; }
    bool available() const;

    void restore();

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void apply(const QString& path);
    Q_INVOKABLE void applyRandom();

signals:
    void currentChanged();

private:
    QStringList searchDirs() const;

    WallpaperModel m_model;
    QString m_current;
};

} // namespace nicoos
