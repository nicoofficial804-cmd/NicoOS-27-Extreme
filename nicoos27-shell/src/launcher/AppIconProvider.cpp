#include "nicoos/AppIconProvider.hpp"
#include "nicoos/ConfigManager.hpp"

#include <QIcon>
#include <QUrl>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QFont>
#include <QCryptographicHash>

namespace nicoos {

AppIconProvider::AppIconProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

bool AppIconProvider::themeHasIcon(const QString& iconName) {
    if (iconName.isEmpty()) return false;
    return !QIcon::fromTheme(iconName).isNull();
}

QString AppIconProvider::makeId(const QString& iconName, const QString& displayName) {
    // La codifica percentuale garantisce che nessuna delle due parti contenga
    // '/', quindi il primo '/' le separa sempre (i nomi delle icone possono
    // essere percorsi assoluti).
    return QString::fromLatin1(QUrl::toPercentEncoding(iconName)) + "/" +
           QString::fromLatin1(QUrl::toPercentEncoding(displayName));
}

QPixmap AppIconProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) {
    const int sepIdx = id.indexOf('/');
    const QString iconName = QUrl::fromPercentEncoding((sepIdx >= 0 ? id.left(sepIdx) : id).toUtf8());
    const QString displayName = QUrl::fromPercentEncoding((sepIdx >= 0 ? id.mid(sepIdx + 1) : id).toUtf8());

    const int px = requestedSize.width() > 0 ? requestedSize.width() : 48;
    const QString cacheKey = QString("%1@%2").arg(id).arg(px);

    if (auto it = m_cache.constFind(cacheKey); it != m_cache.constEnd()) {
        if (size) *size = it->size();
        return *it;
    }

    QPixmap result;
    if (QFileInfo(iconName).isAbsolute()) {
        result = renderFileIcon(iconName, px);
    } else if (themeHasIcon(iconName)) {
        result = renderThemeIcon(iconName, px);
    }
    if (result.isNull()) {
        result = renderFallbackIcon(displayName.isEmpty() ? iconName : displayName, px);
    }

    m_cache.insert(cacheKey, result);
    if (size) *size = result.size();
    return result;
}

QPixmap AppIconProvider::renderFileIcon(const QString& path, int size) const {
    QPixmap pm(path);
    if (pm.isNull()) return {};
    return pm.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPixmap AppIconProvider::renderThemeIcon(const QString& iconName, int size) const {
    QIcon icon = QIcon::fromTheme(iconName);
    if (icon.isNull()) return {};
    return icon.pixmap(size, size);
}

QColor AppIconProvider::colorForName(const QString& name) const {
    // Hash deterministico -> tonalità, così la stessa app ha sempre lo stesso
    // colore tra un'esecuzione e l'altra, mentre app diverse si distribuiscono
    // bene sulla ruota dei colori invece di ammassarsi su una tonalità.
    const QByteArray hash = QCryptographicHash::hash(name.toUtf8(), QCryptographicHash::Md5);
    const int hue = static_cast<unsigned char>(hash[0]) * 360 / 256;

    // Mantieni saturazione/luminosità nella stessa famiglia neon-glass del
    // resto della shell invece di lasciare che l'hash scelga qualcosa (ad
    // esempio un marrone spento) che stonerebbe con la palette d'accento.
    return QColor::fromHsl(hue, 200, 150);
}

QPixmap AppIconProvider::renderFallbackIcon(const QString& appName, int size) const {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    const QColor base = colorForName(appName.isEmpty() ? "?" : appName);
    const QColor accent = ConfigManager::instance().accentColor();
    // Mescola il colore hash della singola app con l'accento configurato
    // dall'utente, così le icone generate sembrano parte della palette di
    // questa shell invece che tessere arcobaleno casuali.
    const QColor blended = QColor::fromRgbF(
        (base.redF() + accent.redF()) / 2.0,
        (base.greenF() + accent.greenF()) / 2.0,
        (base.blueF() + accent.blueF()) / 2.0);

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal margin = size * 0.06;
    const QRectF bounds(margin, margin, size - 2 * margin, size - 2 * margin);

    // Tessera esagonale in vetro, in tinta con il resto di pannello e
    // launcher.
    QPainterPath hex;
    const qreal cx = bounds.center().x();
    const qreal cy = bounds.center().y();
    const qreal r = bounds.width() / 2.0;
    for (int i = 0; i < 6; ++i) {
        const qreal angle = M_PI / 180.0 * (60.0 * i - 90.0);
        const QPointF pt(cx + r * std::cos(angle), cy + r * std::sin(angle));
        if (i == 0) hex.moveTo(pt); else hex.lineTo(pt);
    }
    hex.closeSubpath();

    QLinearGradient fillGrad(bounds.topLeft(), bounds.bottomRight());
    fillGrad.setColorAt(0.0, QColor(10, 16, 22, 235));
    fillGrad.setColorAt(1.0, QColor(18, 26, 34, 235));
    p.setPen(Qt::NoPen);
    p.setBrush(fillGrad);
    p.drawPath(hex);

    p.setPen(QPen(blended, size * 0.035));
    p.setBrush(Qt::NoBrush);
    p.drawPath(hex);

    // Bagliore esterno morbido dietro il glifo, approssimato con un gradiente
    // radiale invece di un vero filtro di sfocatura (economico, nessun giro su
    // QGraphicsEffect necessario per un rendering di pixmap fuori schermo).
    QRadialGradient glow(cx, cy, r * 0.75);
    QColor glowColor = blended;
    glowColor.setAlpha(90);
    glow.setColorAt(0.0, glowColor);
    glowColor.setAlpha(0);
    glow.setColorAt(1.0, glowColor);
    p.setBrush(glow);
    p.setPen(Qt::NoPen);
    p.drawEllipse(bounds.center(), r * 0.75, r * 0.75);

    // Glifo: prima lettera del nome dell'app, maiuscola.
    const QString glyph = appName.trimmed().isEmpty()
        ? QStringLiteral("?")
        : QString(appName.trimmed().at(0)).toUpper();

    QFont font;
    font.setPixelSize(int(size * 0.42));
    font.setWeight(QFont::DemiBold);
    p.setFont(font);
    p.setPen(QColor(235, 249, 255));
    p.drawText(bounds, Qt::AlignCenter, glyph);

    return pixmap;
}

} // namespace nicoos
