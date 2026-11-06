#include "nicoos/ClipboardManager.hpp"
#include "nicoos/ProcessUtil.hpp"

#include <QProcess>

namespace nicoos {

ClipboardManager::ClipboardManager(QObject* parent) : QAbstractListModel(parent) {
    m_available = hasExecutable("cliphist") && hasExecutable("wl-paste") && hasExecutable("wl-copy");
    if (!m_available) return;

    startWatcher({"--type", "text", "--watch", "cliphist", "store"});
    startWatcher({"--type", "image", "--watch", "cliphist", "store"});
    refresh();
}

ClipboardManager::~ClipboardManager() {
    for (auto* p : std::as_const(m_watchers)) {
        p->disconnect(this);
        p->kill();
        p->waitForFinished(300);
    }
}

void ClipboardManager::startWatcher(const QStringList& args) {
    auto* p = new QProcess(this);
    m_watchers.push_back(p);
    p->start("wl-paste", args);
}

int ClipboardManager::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_filtered.size();
}

QVariant ClipboardManager::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_filtered.size()) return {};
    const auto& e = m_filtered[index.row()];
    switch (role) {
        case TextRole:  return e.text;
        case ImageRole: return e.image;
        default:        return {};
    }
}

QHash<int, QByteArray> ClipboardManager::roleNames() const {
    return {{TextRole, "clipText"}, {ImageRole, "clipIsImage"}};
}

void ClipboardManager::setFilterText(const QString& text) {
    if (m_filter == text) return;
    m_filter = text;
    applyFilter();
    emit filterChanged();
}

void ClipboardManager::applyFilter() {
    beginResetModel();
    if (m_filter.trimmed().isEmpty()) {
        m_filtered = m_all;
    } else {
        m_filtered.clear();
        const QString needle = m_filter.trimmed();
        for (const auto& e : std::as_const(m_all)) {
            if (e.text.contains(needle, Qt::CaseInsensitive)) m_filtered.push_back(e);
        }
    }
    endResetModel();
    emit countChanged();
}

void ClipboardManager::refresh() {
    if (!m_available) return;
    runCapture(this, "cliphist", {"list"}, [this](int code, const QString& out) {
        if (code != 0) return;
        m_all.clear();
        const QStringList lines = out.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            if (m_all.size() >= 60) break;
            const int tab = line.indexOf('\t');
            if (tab < 0) continue;
            Entry e;
            e.raw = line;
            QString preview = line.mid(tab + 1).trimmed();
            if (preview.startsWith("[[ binary data")) {
                e.image = true;
                preview = "Immagine  " + preview.mid(15, preview.size() - 18).trimmed();
            }
            if (preview.size() > 160) preview = preview.left(160) + "…";
            e.text = preview;
            m_all.push_back(e);
        }
        applyFilter();
    });
}

void ClipboardManager::select(int row) {
    if (row < 0 || row >= m_filtered.size()) return;
    // Il flusso documentato di cliphist: passa la riga scelta dall'elenco
    // attraverso decode.
    QProcess::startDetached("sh", {"-c", R"(printf '%s\n' "$1" | cliphist decode | wl-copy)",
                                   "sh", m_filtered[row].raw});
}

void ClipboardManager::remove(int row) {
    if (row < 0 || row >= m_filtered.size()) return;
    runCapture(this, "sh", {"-c", R"(printf '%s\n' "$1" | cliphist delete)", "sh", m_filtered[row].raw},
               [this](int, const QString&) { refresh(); });
}

void ClipboardManager::wipe() {
    runCapture(this, "cliphist", {"wipe"}, [this](int, const QString&) { refresh(); });
}

} // namespace nicoos
