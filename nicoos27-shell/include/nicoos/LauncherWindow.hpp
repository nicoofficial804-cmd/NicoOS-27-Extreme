#pragma once

#include "nicoos/LayerSurfaceWindow.hpp"
#include "nicoos/AppIndexer.hpp"
#include "nicoos/FuzzyMatcher.hpp"
#include <QAbstractListModel>
#include <memory>

namespace nicoos {

// Espone i risultati del matching fuzzy al QML come normale list model
// (ruoli: name, comment, iconName, appId), così la ListView del launcher può
// collegarsi direttamente senza collante C++/QML per ogni campo.
class LauncherResultsModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)

public:
    enum Role { NameRole = Qt::UserRole + 1, CommentRole, IconRole, AppIdRole };

    explicit LauncherResultsModel(AppIndexer* indexer, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString query() const { return m_query; }
    void setQuery(const QString& q);

    Q_INVOKABLE void launch(int row);
    Q_INVOKABLE void launchTopResult();

signals:
    void queryChanged();
    void appLaunched();

private:
    void refilter();

    AppIndexer* m_indexer;
    QString m_query;
    QVector<MatchResult> m_filtered;
};

class LauncherWindow : public QObject {
    Q_OBJECT

public:
    explicit LauncherWindow(QObject* parent = nullptr);
    ~LauncherWindow() override;

    void toggle();
    bool isVisible() const;

private:
    void buildWindow();

    std::unique_ptr<AppIndexer> m_indexer;
    std::unique_ptr<LauncherResultsModel> m_model;
    std::unique_ptr<LayerSurfaceWindow> m_window;
};

} // namespace nicoos
