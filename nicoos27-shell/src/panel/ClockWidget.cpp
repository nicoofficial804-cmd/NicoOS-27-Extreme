#include "nicoos/ClockWidget.hpp"

#include <QDateTime>
#include <QLocale>

namespace nicoos {

ClockWidget::ClockWidget(QObject* parent) : QObject(parent) {
    connect(&m_timer, &QTimer::timeout, this, &ClockWidget::tick);
    m_timer.start(1000);
}

QString ClockWidget::time() const {
    return QDateTime::currentDateTime().toString(m_format);
}

QString ClockWidget::date() const {
    return QLocale::system().toString(QDate::currentDate(), QLocale::LongFormat);
}

void ClockWidget::setFormat(const QString& timeFormat) {
    m_format = timeFormat;
    emit tick();
}

} // namespace nicoos
