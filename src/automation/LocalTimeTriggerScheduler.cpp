#include "automation/LocalTimeTriggerScheduler.h"

namespace churchpresenter {

LocalTimeTriggerScheduler::LocalTimeTriggerScheduler(QObject *parent)
    : QObject(parent)
    , m_clock([] { return QDateTime::currentDateTime(); })
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &LocalTimeTriggerScheduler::checkNow);
}

void LocalTimeTriggerScheduler::setClock(std::function<QDateTime()> clock)
{
    if (clock) m_clock = std::move(clock);
}

void LocalTimeTriggerScheduler::start()
{
    checkNow();
    m_timer.start();
}

void LocalTimeTriggerScheduler::stop() { m_timer.stop(); }
bool LocalTimeTriggerScheduler::isActive() const { return m_timer.isActive(); }

void LocalTimeTriggerScheduler::checkNow()
{
    const auto local = m_clock().toLocalTime();
    if (!local.isValid()) return;

    const auto occurrence = QStringLiteral("%1|%2|%3")
        .arg(local.date().toString(Qt::ISODate), local.time().toString(QStringLiteral("HH:mm")))
        .arg(local.offsetFromUtc());
    if (occurrence == m_lastOccurrence) return;
    m_lastOccurrence = occurrence;

    const QVariantMap payload{
        {QStringLiteral("localDate"), local.date().toString(Qt::ISODate)},
        {QStringLiteral("localTime"), local.time().toString(QStringLiteral("HH:mm"))},
        {QStringLiteral("hour"), local.time().hour()},
        {QStringLiteral("minute"), local.time().minute()},
        {QStringLiteral("dayOfWeek"), local.date().dayOfWeek()},
        {QStringLiteral("timezoneOffsetSeconds"), local.offsetFromUtc()},
    };
    emit localTimeOccurred(
        payload,
        QStringLiteral("time:%1").arg(local.toUTC().toString(QStringLiteral("yyyyMMddTHHmmZ"))));
}

} // namespace churchpresenter
