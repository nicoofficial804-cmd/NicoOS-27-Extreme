#include "nicoos/WindowPreviewCapture.hpp"
#include "nicoos/ConfigManager.hpp"

#include <QPainter>
#include <QLinearGradient>
#include <QCryptographicHash>
#include <QFont>

namespace nicoos {

// ---------- WindowPreviewCapture ----------

WindowPreviewCapture::WindowPreviewCapture(QObject* parent) : QObject(parent) {
    // m_protocolBound resta false finche' il binding hyprland-toplevel-export-v1
    // (o wlr-screencopy) non viene compilato; vedi il commento
    // nell'header per il motivo per cui qui non e' incluso direttamente.
}

QImage WindowPreviewCapture::cachedOrPlaceholder(const QString& windowAddress) {
    QMutexLocker lock(&m_cacheMutex);
    if (auto it = m_cache.constFind(windowAddress); it != m_cache.constEnd()) {
        return *it;
    }

    // Ancora nulla di catturato per questo indirizzo — restituisci subito
    // un'immagine trasparente neutra invece di bloccarti; requestPreview()
    // (chiamata da QML alla creazione del delegate) popola l'elemento
    // reale in modo asincrono e il delegate richiede di nuovo quando
    // viene emesso previewReady().
    QImage blank(320, 200, QImage::Format_ARGB32_Premultiplied);
    blank.fill(Qt::transparent);
    return blank;
}

void WindowPreviewCapture::requestPreview(const QString& windowAddress, const QString& windowClass) {
    {
        QMutexLocker lock(&m_cacheMutex);
        if (m_cache.contains(windowAddress)) {
            emit previewReady(windowAddress, m_cache.value(windowAddress));
            return;
        }
    }

    if (m_protocolBound) {
        requestFrame(windowAddress);
        return;
    }

    // Percorso segnaposto sincrono — immediato, nessuno scambio col compositor.
    const QImage img = placeholderFor(windowClass);
    {
        QMutexLocker lock(&m_cacheMutex);
        m_cache.insert(windowAddress, img);
    }
    emit previewReady(windowAddress, img);
}

void WindowPreviewCapture::requestFrame(const QString& /*windowAddress*/) {
    // Punto di aggancio per il percorso di cattura reale. Un'implementazione
    // completa collega hyprland_toplevel_export_manager_v1, chiama
    // ::capture_toplevel_with_wlr_toplevel_handle() per l'handle
    // corrispondente, rilegge il wl_buffer risultante (un buffer dmabuf
    // o shm a seconda della configurazione del compositor) in una QImage
    // ed emette previewReady() dal callback "ready" del buffer. Tutto cio'
    // richiede binding generati con wayland-scanner per l'XML del
    // protocollo di Hyprland in fase di build, volutamente tenuti fuori
    // da questa unita' di traduzione portabile — vedi WindowPreviewCapture.hpp.
}

QImage WindowPreviewCapture::placeholderFor(const QString& windowClass) const {
    const int w = 320, h = 200;
    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);

    // Tinta deterministica per applicazione, cosi' le finestre ripetute
    // della stessa app risultano coerenti nella panoramica.
    const QByteArray hash = QCryptographicHash::hash(windowClass.toUtf8(), QCryptographicHash::Md5);
    const int hue = static_cast<unsigned char>(hash[0]) * 360 / 256;
    QColor tint = QColor::fromHsl(hue, 160, 60);

    QLinearGradient grad(0, 0, w, h);
    grad.setColorAt(0.0, QColor(14, 20, 26));
    grad.setColorAt(1.0, QColor(10, 14, 18));
    p.fillRect(img.rect(), grad);

    // Debole riflesso diagonale "vetro", coerente con il resto della shell.
    QLinearGradient sheen(0, 0, w * 0.6, h);
    QColor sheenColor = tint;
    sheenColor.setAlpha(28);
    sheen.setColorAt(0.0, sheenColor);
    sheenColor.setAlpha(0);
    sheen.setColorAt(1.0, sheenColor);
    p.fillRect(img.rect(), sheen);

    p.setPen(QPen(tint, 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(img.rect().adjusted(1, 1, -1, -1), 10, 10);

    // Glifo dell'app, centrato — riusa lo stesso linguaggio "prima lettera
    // in un esagono luminoso" delle icone generate dal launcher, cosi' una
    // finestra senza cattura reale resta riconoscibile a colpo d'occhio
    // come "quella app" invece di essere un rettangolo grigio vuoto.
    const QString glyph = windowClass.trimmed().isEmpty()
        ? QStringLiteral("?")
        : QString(windowClass.trimmed().at(0)).toUpper();

    QFont font;
    font.setPixelSize(44);
    font.setWeight(QFont::Light);
    p.setFont(font);
    QColor glyphColor = tint.lighter(160);
    glyphColor.setAlpha(150);
    p.setPen(glyphColor);
    p.drawText(img.rect(), Qt::AlignCenter, glyph);

    return img;
}

// ---------- WindowPreviewImageProvider ----------

WindowPreviewImageProvider::WindowPreviewImageProvider(WindowPreviewCapture* capture)
    : QQuickImageProvider(QQuickImageProvider::Pixmap), m_capture(capture) {}

QPixmap WindowPreviewImageProvider::requestPixmap(const QString& id, QSize* size, const QSize&) {
    const QImage img = m_capture->cachedOrPlaceholder(id);
    if (size) *size = img.size();
    return QPixmap::fromImage(img);
}

} // namespace nicoos
