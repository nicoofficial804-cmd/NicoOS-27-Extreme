#pragma once

#include "nicoos/PopupWindow.hpp"
#include "nicoos/NotificationDaemon.hpp"
#include <QAbstractListModel>
#include <QTimer>
#include <memory>

namespace nicoos {

// Modello di supporto per le schede di notifica impilate mostrate in alto a
// destra. Gestisce la coda, i timer di chiusura automatica per ogni voce, ed
// espone aggiunta/rimozione al QML perché le animazioni di entrata/uscita
// possano agganciarvisi.
class NotificationQueueModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role { IdRole = Qt::UserRole + 1, AppNameRole, SummaryRole, BodyRole, IconRole };

    explicit NotificationQueueModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void push(const NotificationEntry& entry);
    Q_INVOKABLE void dismiss(uint id);

private:
    QVector<NotificationEntry> m_entries;
};

class NotificationWindow : public QObject {
    Q_OBJECT

public:
    explicit NotificationWindow(QObject* parent = nullptr);
    ~NotificationWindow() override;

public slots:
    void enqueue(const NotificationEntry& entry);
    void dismiss(uint id);

private:
    void buildWindow();
    void updateVisibility();

    std::unique_ptr<NotificationQueueModel> m_model;
    std::unique_ptr<PopupWindow> m_popup;
};

} // namespace nicoos
