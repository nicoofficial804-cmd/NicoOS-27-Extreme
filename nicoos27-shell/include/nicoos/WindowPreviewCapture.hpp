#pragma once

#include <QObject>
#include <QQuickImageProvider>
#include <QImage>
#include <QHash>
#include <QMutex>

namespace nicoos {

// Produce anteprime quasi dal vivo delle finestre per la panoramica
// "Extreme" delle aree di lavoro.
//
// Volutamente NON è essa stessa un QQuickImageProvider: QQuickImageProvider e
// QObject non possono essere combinati con l'ereditarietà multipla nella
// stessa classe (moc genera un riferimento QObject a base ambigua appena si
// aggiunge una seconda base non correlata: è un limite di Qt/moc, non
// qualcosa che si risolva riordinando le basi). Segnali/slot (previewReady,
// requestPreview come Q_INVOKABLE) richiedono QObject; servire
// "image://windowpreview/<address>" richiede QQuickImageProvider. Le due
// responsabilità sono quindi separate: questa classe è la metà QObject
// (possiede la cache, fa il lavoro di cattura vero, emette previewReady()), e
// WindowPreviewImageProvider qui sotto è un sottile QQuickImageProvider non
// QObject che legge dalla stessa cache tramite un puntatore grezzo a questa
// istanza.
//
// Il vero contenuto dello schermo per singola finestra su Wayland richiede o:
//   (a) il protocollo specifico del compositore wlr-screencopy-unstable-v1
//       legato a una specifica uscita/regione, che dà un frame dell'intera
//       uscita invece dei pixel di una sola finestra senza ulteriore aiuto di
//       compositing, oppure
//   (b) il protocollo hyprland-toplevel-export-v1 di Hyprland, che invece
//       esporta direttamente le singole superfici toplevel: la soluzione
//       naturale qui, dato che questa shell è già specifica per Hyprland.
//
// Questa classe è scritta concettualmente contro (b): requestPreview() è
// l'unico punto d'ingresso che la panoramica chiama, e il vero binding
// zwlr/hyprland-export (che richiede header di protocollo generati da
// wayland-scanner in fase di compilazione — vedi la guardia LAYER_SHELL di
// CMakeLists.txt per lo schema equivalente) si innesta in requestFrame().
// Finché quel binding non è collegato, requestPreview() ripiega su un
// segnaposto tematizzato, così la panoramica è pienamente utilizzabile senza,
// e passa in silenzio ad anteprime reali quando il collante del protocollo
// arriva.
class WindowPreviewCapture : public QObject {
    Q_OBJECT

public:
    explicit WindowPreviewCapture(QObject* parent = nullptr);

    // Attivazione asincrona: assicura che esista in cache un'immagine
    // (eventualmente un segnaposto) per questa finestra, poi emette
    // previewReady() così i delegate QML sanno di poter richiedere di nuovo
    // l'URL image://.
    Q_INVOKABLE void requestPreview(const QString& windowAddress, const QString& windowClass);

    bool hasRealCapture() const { return m_protocolBound; }

    // Chiamata da WindowPreviewImageProvider::requestPixmap(), che non può
    // toccare m_cache direttamente perché non è un membro del QObject di questa
    // classe (è un oggetto separato, non QObject). Sicura rispetto ai thread: le
    // richieste di QQuickImageProvider possono arrivare fuori dal thread GUI
    // quando il tipo di immagine del provider richiede il caricamento su
    // thread, quindi questo e ogni altro accesso a m_cache passano da
    // m_cacheMutex.
    QImage cachedOrPlaceholder(const QString& windowAddress);

signals:
    void previewReady(const QString& windowAddress, const QImage& image);

private:
    QImage placeholderFor(const QString& windowClass) const;
    void requestFrame(const QString& windowAddress);

    bool m_protocolBound{false};
    QMutex m_cacheMutex;
    QHash<QString, QImage> m_cache;
};

// Sottile adattatore QQuickImageProvider, volutamente NON un QObject (vedi il
// commento di classe di WindowPreviewCapture per il perché i due non possano
// essere combinati). Tiene solo un puntatore grezzo all'indietro verso il
// WindowPreviewCapture che possiede la cache e la logica di cattura vere;
// WorkspaceOverview possiede entrambi e garantisce che l'istanza di cattura
// sopravviva a questo provider (la QQmlEngine che possiede questo provider
// viene distrutta insieme alla stessa gerarchia finestra/genitore che possiede
// l'istanza di cattura).
class WindowPreviewImageProvider : public QQuickImageProvider {
public:
    explicit WindowPreviewImageProvider(WindowPreviewCapture* capture);

    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
    WindowPreviewCapture* m_capture;
};

} // namespace nicoos
