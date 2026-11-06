#pragma once

#include <QQuickImageProvider>
#include <QString>
#include <QColor>
#include <QHash>

namespace nicoos {

// Serve "image://nicoosicon/<id>" al QML. Costruisci gli id con makeId().
//
// Ordine di risoluzione:
//   1. un percorso assoluto di icona (alcuni file .desktop ne usano uno),
//   2. il tema di icone (ricerca freedesktop tramite QIcon::fromTheme),
//   3. un esagono neon in vetro generato con l'iniziale dell'app, così nessuna
//      app mostra mai una tessera vuota.
class AppIconProvider : public QQuickImageProvider {
public:
    AppIconProvider();

    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

    // Codifica nome dell'icona + nome visualizzato in un unico id sicuro per
    // gli URL.
    static QString makeId(const QString& iconName, const QString& displayName);
    static bool themeHasIcon(const QString& iconName);

private:
    QPixmap renderThemeIcon(const QString& iconName, int size) const;
    QPixmap renderFileIcon(const QString& path, int size) const;
    QPixmap renderFallbackIcon(const QString& appName, int size) const;
    QColor colorForName(const QString& name) const;

    mutable QHash<QString, QPixmap> m_cache;
};

} // namespace nicoos
