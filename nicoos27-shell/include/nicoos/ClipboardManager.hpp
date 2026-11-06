#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

class QProcess;

namespace nicoos {

// Cronologia degli appunti basata su `cliphist`. Mentre la shell è in
// esecuzione mantiene vivo `wl-paste --watch cliphist store` (testo e
// immagini), e il modello elenca le voci memorizzate. Selezionando una voce
// la si rimette negli appunti con wl-copy.
class ClipboardManager : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterChanged)

public:
    enum Role { TextRole = Qt::UserRole + 1, ImageRole };

    explicit ClipboardManager(QObject* parent = nullptr);
    ~ClipboardManager() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool available() const { return m_available; }
    int count() const { return m_filtered.size(); }
    QString filterText() const { return m_filter; }
    void setFilterText(const QString& text);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void select(int row);
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void wipe();

signals:
    void countChanged();
    void filterChanged();

private:
    struct Entry {
        QString raw;    // riga completa di `cliphist list` ("<id>\t<anteprima>")
        QString text;
        bool image{false};
    };

    void applyFilter();
    void startWatcher(const QStringList& args);

    bool m_available{false};
    QString m_filter;
    QVector<Entry> m_all;
    QVector<Entry> m_filtered;
    QVector<QProcess*> m_watchers;
};

} // namespace nicoos
