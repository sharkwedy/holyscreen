#include "presentation/OverlayController.h"

#include <algorithm>

namespace churchpresenter {

namespace {
QString formatDuration(int seconds)
{
    const auto hours = seconds / 3600;
    const auto minutes = (seconds % 3600) / 60;
    const auto remainingSeconds = seconds % 60;
    return hours > 0
        ? QStringLiteral("%1:%2:%3").arg(hours, 2, 10, QLatin1Char('0'))
              .arg(minutes, 2, 10, QLatin1Char('0')).arg(remainingSeconds, 2, 10, QLatin1Char('0'))
        : QStringLiteral("%1:%2").arg(minutes, 2, 10, QLatin1Char('0'))
              .arg(remainingSeconds, 2, 10, QLatin1Char('0'));
}
}

OverlayController::OverlayController(QObject *parent) : QObject(parent)
{
    m_tick.setInterval(1000);
    connect(&m_tick, &QTimer::timeout, this, &OverlayController::advanceOneSecond);
}
QString OverlayController::message() const { return m_message; }
QString OverlayController::alert() const { return m_alert; }
QString OverlayController::lowerThirdTitle() const { return m_lowerThirdTitle; }
QString OverlayController::lowerThirdSubtitle() const { return m_lowerThirdSubtitle; }
QString OverlayController::countdownText() const { return formatDuration(m_countdownSeconds); }
QString OverlayController::stopwatchText() const { return formatDuration(m_stopwatchSeconds); }
bool OverlayController::countdownRunning() const { return m_countdownRunning; }
bool OverlayController::stopwatchRunning() const { return m_stopwatchRunning; }
void OverlayController::setMessage(const QString &message)
{
    const auto value = message.trimmed(); if (value == m_message) return;
    m_message = value; emit changed();
}
void OverlayController::setAlert(const QString &alert)
{
    const auto value = alert.trimmed(); if (value == m_alert) return;
    m_alert = value; emit changed();
}
void OverlayController::setLowerThird(const QString &title, const QString &subtitle)
{
    const auto normalizedTitle = title.trimmed();
    const auto normalizedSubtitle = subtitle.trimmed();
    if (normalizedTitle == m_lowerThirdTitle && normalizedSubtitle == m_lowerThirdSubtitle) return;
    m_lowerThirdTitle = normalizedTitle; m_lowerThirdSubtitle = normalizedSubtitle; emit changed();
}
void OverlayController::startCountdown(int seconds)
{
    m_countdownSeconds = std::max(0, seconds);
    m_countdownRunning = m_countdownSeconds > 0;
    if (m_countdownRunning || m_stopwatchRunning) m_tick.start();
    emit changed();
}
void OverlayController::stopCountdown()
{
    if (!m_countdownRunning) return;
    m_countdownRunning = false;
    if (!m_stopwatchRunning) m_tick.stop();
    emit changed();
}
void OverlayController::startStopwatch()
{
    if (m_stopwatchRunning) return;
    m_stopwatchRunning = true; m_tick.start(); emit changed();
}
void OverlayController::pauseStopwatch()
{
    if (!m_stopwatchRunning) return;
    m_stopwatchRunning = false;
    if (!m_countdownRunning) m_tick.stop();
    emit changed();
}
void OverlayController::resetStopwatch()
{
    m_stopwatchRunning = false; m_stopwatchSeconds = 0;
    if (!m_countdownRunning) m_tick.stop();
    emit changed();
}
void OverlayController::advanceOneSecond()
{
    if (m_countdownRunning && m_countdownSeconds > 0) {
        --m_countdownSeconds;
        if (m_countdownSeconds == 0) m_countdownRunning = false;
    }
    if (m_stopwatchRunning) ++m_stopwatchSeconds;
    if (!m_countdownRunning && !m_stopwatchRunning) m_tick.stop();
    emit changed();
}

} // namespace churchpresenter
