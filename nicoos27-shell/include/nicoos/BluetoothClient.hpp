#pragma once

#include <QObject>
#include <QDBusInterface>
#include <memory>

namespace nicoos {

// Client minimale per BlueZ (org.bluez): riporta lo stato di accensione
// dell'adattatore e permette al centro di controllo di attivarlo o
// disattivarlo. Un'interfaccia completa di associazione/ricerca dispositivi è
// fuori dal perimetro del centro di controllo della shell (delegata a un'app
// dedicata alle impostazioni Bluetooth tramite l'azione "Informazioni di
// sistema"), coerentemente con il punto "Bluetooth" delle specifiche, che è
// una voce di stato/attivazione e non un gestore completo di dispositivi.
class BluetoothClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool powered READ isPowered NOTIFY poweredChanged)
    Q_PROPERTY(bool available READ isAvailable CONSTANT)

public:
    explicit BluetoothClient(QObject* parent = nullptr);

    bool isPowered() const { return m_powered; }
    bool isAvailable() const { return m_available; }

    Q_INVOKABLE void setPowered(bool on);

signals:
    void poweredChanged();

private slots:
    void refresh();

private:
    std::unique_ptr<QDBusInterface> m_props;
    QString m_adapterPath;
    bool m_powered{false};
    bool m_available{false};
};

} // namespace nicoos
