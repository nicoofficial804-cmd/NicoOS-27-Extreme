#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

namespace nicoos {

// Piccolo helper esposto al QML per formattare data/ora in base alla locale
// andando oltre ciò che Qt.formatDateTime offre comodamente in linea (ad
// esempio un'etichetta separata del "giorno relativo" per l'eventuale vista
// cronologica della panoramica delle aree di lavoro). L'orologio dal vivo del
// pannello è gestito direttamente in Panel.qml con un Timer +
// Qt.formatDateTime, perché non richiede un giro in C++; questa classe esiste
// per i consumatori che vogliono una formattazione più ricca di quanto possa
// esprimere una singola stringa di formato.
class ClockWidget : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString time READ time NOTIFY tick)
    Q_PROPERTY(QString date READ date NOTIFY tick)

public:
    explicit ClockWidget(QObject* parent = nullptr);

    QString time() const;
    QString date() const;

    Q_INVOKABLE void setFormat(const QString& timeFormat);

signals:
    void tick();

private:
    QTimer m_timer;
    QString m_format{"HH:mm"};
};

} // namespace nicoos
