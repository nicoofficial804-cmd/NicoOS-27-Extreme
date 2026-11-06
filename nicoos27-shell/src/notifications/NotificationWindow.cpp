#include "nicoos/NotificationWindow.hpp"
#include "nicoos/ConfigManager.hpp"

#include <QQmlContext>
#include <QTimer>

namespace nicoos {

// ---------- NotificationQueueModel ----------

NotificationQueueModel::NotificationQueueModel(QObject* parent) : QAbstractListModel(parent) {}

int NotificationQueueModel::rowCount(const QModelIndex&) const {
    return m_entries.size();
}

QVariant NotificationQueueModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_entries.size()) return {};
    const auto& e = m_entries[index.row()];
    switch (role) {
        case IdRole: return e.id;
        case AppNameRole: return e.appName;
        case SummaryRole: return e.summary;
        case BodyRole: return e.body;
        case IconRole: return e.iconName;
        default: return {};
    }
}

QHash<int, QByteArray> NotificationQueueModel::roleNames() const {
    return {
        {IdRole, "notifId"},
        {AppNameRole, "appName"},
        {SummaryRole, "summary"},
        {BodyRole, "body"},
        {IconRole, "iconName"},
    };
}

void NotificationQueueModel::push(const NotificationEntry& entry) {
    // Sostituzione sul posto se questo id esiste gia' (da specifica, Notify
    // con un replacesId corrispondente aggiorna invece di duplicare).
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].id == entry.id) {
            m_entries[i] = entry;
            emit dataChanged(index(i), index(i));
            return;
        }
    }

    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size());
    m_entries.push_back(entry);
    endInsertRows();
}

void NotificationQueueModel::dismiss(uint id) {
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].id == id) {
            beginRemoveRows(QModelIndex(), i, i);
            m_entries.remove(i);
            endRemoveRows();
            return;
        }
    }
}

// ---------- NotificationWindow ----------

NotificationWindow::NotificationWindow(QObject* parent) : QObject(parent) {
    m_model = std::make_unique<NotificationQueueModel>();
    buildWindow();
}

NotificationWindow::~NotificationWindow() = default;

void NotificationWindow::buildWindow() {
    // Ancorata accanto al pannello (in alto a destra per impostazione
    // predefinita). La superficie viene mappata solo finche' ci sono
    // notifiche, cosi' l'area trasparente non blocca i clic sulle
    // finestre sottostanti.
    m_popup = std::make_unique<PopupWindow>(QUrl("qrc:/nicoos/qml/NotificationStack.qml"),
                                            PopupPlacement::PanelRight, QSize(340, 520),
                                            KeyboardMode::None);
    m_popup->setExclusive(false);
    m_popup->context()->setContextProperty("notificationModel", m_model.get());
    m_popup->load();

    connect(m_model.get(), &QAbstractItemModel::rowsInserted, this, [this] { updateVisibility(); });
    connect(m_model.get(), &QAbstractItemModel::rowsRemoved, this, [this] { updateVisibility(); });
}

void NotificationWindow::updateVisibility() {
    if (m_model->rowCount() > 0) {
        if (!m_popup->isVisible()) m_popup->show();
    } else {
        // Lascia il tempo all'animazione di uscita prima di smappare.
        QTimer::singleShot(450, this, [this] {
            if (m_model->rowCount() == 0) m_popup->hide();
        });
    }
}

void NotificationWindow::enqueue(const NotificationEntry& entry) {
    m_model->push(entry);

    if (entry.timeoutMs > 0) {
        QTimer::singleShot(entry.timeoutMs, this, [this, id = entry.id] {
            dismiss(id);
        });
    }
}

void NotificationWindow::dismiss(uint id) {
    m_model->dismiss(id);
}

} // namespace nicoos
