#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace nicoos {

// Elenco delle reti Wi-Fi e connessione, tramite `nmcli` (NetworkManager).
// Usato come list model dalla scheda di rete del centro di controllo.
class WifiScanner : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role { SsidRole = Qt::UserRole + 1, SignalRole, SecureRole, ActiveRole };

    explicit WifiScanner(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool available() const { return m_available; }
    bool busy() const { return m_busy; }
    int count() const { return m_networks.size(); }

    Q_INVOKABLE void scan();
    Q_INVOKABLE void connectTo(const QString& ssid);
    Q_INVOKABLE void connectWithPassword(const QString& ssid, const QString& password);

signals:
    void busyChanged();
    void countChanged();
    // La connessione a una rete protetta è fallita: chiedi la password
    // all'utente.
    void passwordRequired(const QString& ssid);
    void connectFinished(const QString& ssid, bool ok);

private:
    struct Network {
        QString ssid;
        int signal{};
        bool secure{};
        bool active{};
    };

    void setBusy(bool busy);
    bool isSecure(const QString& ssid) const;
    void finishConnect(const QString& ssid, int exitCode, bool hadPassword);

    bool m_available{false};
    bool m_busy{false};
    QVector<Network> m_networks;
};

} // namespace nicoos
