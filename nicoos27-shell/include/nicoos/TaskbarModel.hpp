#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QTimer>
#include <QVector>
#include <memory>

#include "nicoos/AppIndexer.hpp"

namespace nicoos {

class HyprlandEventListener;

struct TaskbarEntry {
    QString address;
    QString windowClass;
    QString title;
    QString iconKey;      // id per image://nicoosicon/
    int workspaceId{};
    bool active{};
    bool minimized{};

    bool operator==(const TaskbarEntry&) const = default;
};

// Lista finestre in stile Windows per il pannello. Le righe vengono
// aggiornate in modo incrementale (inserimento / rimozione / dataChanged)
// invece che con un reset, così le transizioni add/remove/displaced del QML
// scorrono fluide. Gli aggiornamenti sono a eventi (socket2 di Hyprland) e
// raggruppati; nulla fa polling.
//
// Le finestre "minimizzate" vengono parcheggiate nell'area di lavoro
// special:minimized (Hyprland non ha una vera minimizzazione) e mostrate
// attenuate.
class TaskbarModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        AddressRole = Qt::UserRole + 1,
        ClassRole,
        TitleRole,
        IconKeyRole,
        WorkspaceRole,
        ActiveRole,
        MinimizedRole,
    };

    explicit TaskbarModel(HyprlandEventListener* events, QObject* parent = nullptr);
    ~TaskbarModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_entries.size(); }

    // Clic sinistro: focus; il clic sulla finestra attiva la minimizza; il
    // clic su una finestra minimizzata la ripristina nell'area di lavoro
    // corrente.
    Q_INVOKABLE void activate(const QString& address);
    Q_INVOKABLE void close(const QString& address);

public slots:
    void refresh();

signals:
    void countChanged();

private:
    QString iconKeyFor(const QString& windowClass) const;
    void buildIconLookup();
    int indexOfAddress(const QString& address) const;

    std::unique_ptr<AppIndexer> m_indexer;
    QHash<QString, DesktopApp> m_lookup;   // classe/id/exec in minuscolo -> app
    QVector<TaskbarEntry> m_entries;
    QTimer m_debounce;
};

} // namespace nicoos
