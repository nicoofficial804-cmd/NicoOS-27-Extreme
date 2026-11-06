#pragma once

#include "nicoos/LayerSurfaceWindow.hpp"
#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <memory>

namespace nicoos {

class HyprlandEventListener;
class WindowPreviewCapture;

struct WorkspaceRow {
    int id{};
    QString name;
    int windowCount{};
    bool active{};
    bool hasFullscreen{};
};

struct WindowRow {
    QString address;
    QString windowClass;
    QString title;
    int workspaceId{};
    bool floating{};
    bool fullscreen{};
};

class WorkspaceListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role { IdRole = Qt::UserRole + 1, NameRole, WindowCountRole, ActiveRole, FullscreenRole };

    explicit WorkspaceListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void refresh();
    Q_INVOKABLE void activate(int workspaceId);
    Q_INVOKABLE int workspaceIdAt(int row) const;
    // Navigazione con rotellina/tastiera: +1 = area successiva, -1 =
    // precedente. Rispetta [behavior] workspace_wrap_around.
    Q_INVOKABLE void activateRelative(int delta);

private:
    QVector<WorkspaceRow> m_rows;
};

// Modello piatto di ogni finestra di ogni area di lavoro, indipendente da
// WorkspaceListModel, così la modalità "tutte le finestre, un'unica grande
// griglia" della panoramica e il raggruppamento per area possono leggere dagli
// stessi dati dal vivo senza duplicare le chiamate IPC a Hyprland.
class WindowListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterChanged)

public:
    enum Role {
        AddressRole = Qt::UserRole + 1,
        ClassRole,
        TitleRole,
        WorkspaceIdRole,
        FloatingRole,
        PreviewSourceRole,
    };

    explicit WindowListModel(WindowPreviewCapture* previews, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void refresh();

    QString filterText() const { return m_filterText; }
    void setFilterText(const QString& text);

    // Invocata dal QML quando un trascinamento su una scheda di area di lavoro
    // è completato.
    Q_INVOKABLE void moveWindowToWorkspace(const QString& address, int workspaceId);
    Q_INVOKABLE void focusWindow(const QString& address);
    Q_INVOKABLE void closeWindow(const QString& address);

    // Restituisce ogni finestra dell'area di lavoro indicata come lista di
    // QVariantMap {winAddress, winClass, winTitle, winWorkspaceId}, pronta da
    // usare come sorgente model.data di un ListModel/Repeater QML: permette a
    // ogni scheda di area di lavoro in WorkspaceOverview.qml di costruire la
    // propria piccola lista di finestre senza codificare a mano nel QML i
    // numeri di ruolo interni di questo modello.
    Q_INVOKABLE QVariantList windowsForWorkspace(int workspaceId) const;

signals:
    void filterChanged();

private:
    QVector<WindowRow> allRows() const;
    void applyFilter();

    WindowPreviewCapture* m_previews;
    QVector<WindowRow> m_all;
    QVector<WindowRow> m_filtered;
    QString m_filterText;
};

// Panoramica "Extreme": un'unica vista combinata che mostra ogni area di
// lavoro come scheda con anteprime quasi dal vivo delle sue finestre,
// trascinamento tra le schede delle aree di lavoro, e un'unica casella di
// ricerca che filtra le finestre per titolo/classe nell'intera sessione:
// navigazione delle finestre in stile Mission Control e cambio di area di
// lavoro in stile Attività di GNOME fusi in un'unica superficie invece di due
// viste separate. Legge lo stato solo tramite HyprlandIPC/HyprlandEventListener;
// non tocca mai direttamente il rendering del compositore, tranne che tramite
// l'hook opzionale di esportazione toplevel di WindowPreviewCapture.
class WorkspaceOverview : public QObject {
    Q_OBJECT

public:
    explicit WorkspaceOverview(HyprlandEventListener* events, QObject* parent = nullptr);
    ~WorkspaceOverview() override;

    void toggle();

private:
    void buildWindow();
    void refreshAll();

    HyprlandEventListener* m_events;
    // Posseduto normalmente da questa classe: WindowPreviewCapture è un QObject
    // (cache + segnali), non legato alla durata del provider di immagini della
    // QQmlEngine. Il WindowPreviewImageProvider separato creato in
    // buildWindow() NON è un QObject — viene passato a
    // QQmlEngine::addImageProvider(), che prende possesso solo di *quello*
    // oggetto, e tiene semplicemente un puntatore grezzo all'indietro verso
    // questo (vedi WindowPreviewCapture.hpp per il perché della separazione).
    std::unique_ptr<WindowPreviewCapture> m_previews;
    std::unique_ptr<WorkspaceListModel> m_workspaceModel;
    std::unique_ptr<WindowListModel> m_windowModel;
    std::unique_ptr<LayerSurfaceWindow> m_window;
};

} // namespace nicoos
