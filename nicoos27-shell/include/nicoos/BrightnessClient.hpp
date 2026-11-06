#pragma once

#include <QObject>
#include <QString>

namespace nicoos {

// Retroilluminazione dello schermo. Scrivere
// /sys/class/backlight/*/brightness richiede normalmente root o il gruppo
// "video" più una regola udev, quindi una modifica viene applicata in
// quest'ordine:
//   1. una scrittura diretta in sysfs, se il file è scrivibile;
//   2. la chiamata D-Bus SetBrightness di systemd-logind (nessun permesso
//      aggiuntivo per la sessione attiva: è ciò che funziona subito);
//   3. `brightnessctl`, se installato.
// Il valore dello slider viene riportato indietro solo se tutti i metodi
// hanno fallito.
class BrightnessClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(int brightnessPercent READ brightnessPercent NOTIFY brightnessChanged)
    Q_PROPERTY(bool available READ isAvailable CONSTANT)

public:
    // `backlightRoot` è un parametro solo perché i test possano puntarlo
    // altrove.
    explicit BrightnessClient(QObject* parent = nullptr,
                              const QString& backlightRoot = QStringLiteral("/sys/class/backlight"));

    int brightnessPercent() const { return m_percent; }
    bool isAvailable() const { return m_available; }

    Q_INVOKABLE void setBrightnessPercent(int percent);
    // Rilegge il valore corrente (il centro di controllo la chiama
    // all'apertura, dato che sysfs non notifica i cambiamenti dei tasti
    // hardware).
    Q_INVOKABLE void refresh();

signals:
    void brightnessChanged();

private:
    void detectDevice(const QString& root);
    int readRaw() const;
    void pump();
    void writeRaw(int raw);
    void finishWrite(bool ok);

    QString m_name;
    QString m_devicePath;
    int m_max{1};
    int m_percent{100};
    bool m_available{false};
    bool m_busy{false};
    int m_wantedRaw{-1};
};

} // namespace nicoos
