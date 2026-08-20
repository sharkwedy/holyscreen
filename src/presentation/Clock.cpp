#include "presentation/Clock.h"

namespace churchpresenter {

QDateTime SystemClock::now() const
{
    return QDateTime::currentDateTime();
}

ClockController::ClockController(std::unique_ptr<IClock> clock, QObject *parent)
    : QObject(parent)
    , m_clock(std::move(clock))
{
    m_timer.setInterval(250);
    connect(&m_timer, &QTimer::timeout, this, &ClockController::refresh);
    m_timer.start();
    refresh();
}

QString ClockController::text() const { return m_text; }
QString ClockController::format() const { return m_format; }

void ClockController::setFormat(const QString &format)
{
    const auto normalized = (format == QStringLiteral("24h-seconds") || format == QStringLiteral("12h"))
        ? format : QStringLiteral("24h");
    if (m_format == normalized) return;
    m_format = normalized;
    emit formatChanged();
    refresh();
}

void ClockController::refresh()
{
    const auto updated = m_clock->now().toString(qtFormat(m_format));
    if (updated == m_text) return;
    m_text = updated;
    emit textChanged();
}

QString ClockController::qtFormat(const QString &format)
{
    if (format == QStringLiteral("24h-seconds")) return QStringLiteral("HH:mm:ss");
    if (format == QStringLiteral("12h")) return QStringLiteral("hh:mm AP");
    return QStringLiteral("HH:mm");
}

} // namespace churchpresenter
