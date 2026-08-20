#include "presentation/TimedMediaPlayback.h"

#include <algorithm>

namespace churchpresenter {

TimedMediaPlayback::TimedMediaPlayback(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(25);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &TimedMediaPlayback::tick);
}

bool TimedMediaPlayback::active() const { return m_active; }
bool TimedMediaPlayback::playing() const { return m_playing; }

int TimedMediaPlayback::positionMs() const
{
    if (!m_active && m_basePositionMs >= m_durationMs) return m_durationMs;
    const auto position = m_basePositionMs + (m_playing ? static_cast<int>(m_elapsed.elapsed()) : 0);
    return std::clamp(position, 0, m_durationMs);
}

int TimedMediaPlayback::durationMs() const { return m_durationMs; }

void TimedMediaPlayback::start(int durationMs)
{
    m_durationMs = std::max(1, durationMs);
    m_basePositionMs = 0;
    m_active = true;
    m_playing = true;
    m_elapsed.restart();
    m_timer.start();
    emit stateChanged();
    emit positionChanged();
}

void TimedMediaPlayback::pause()
{
    if (!m_active || !m_playing) return;
    m_basePositionMs = positionMs();
    m_playing = false;
    m_timer.stop();
    emit positionChanged();
    emit stateChanged();
}

void TimedMediaPlayback::resume()
{
    if (!m_active || m_playing || m_basePositionMs >= m_durationMs) return;
    m_playing = true;
    m_elapsed.restart();
    m_timer.start();
    emit stateChanged();
}

void TimedMediaPlayback::stop()
{
    const bool changed = m_active || m_playing || m_basePositionMs != 0 || m_durationMs != 0;
    m_timer.stop();
    m_active = false;
    m_playing = false;
    m_basePositionMs = 0;
    m_durationMs = 0;
    if (changed) {
        emit positionChanged();
        emit stateChanged();
    }
}

void TimedMediaPlayback::seek(int positionMs)
{
    if (!m_active) return;
    m_basePositionMs = std::clamp(positionMs, 0, m_durationMs);
    if (m_playing) m_elapsed.restart();
    emit positionChanged();
    if (m_basePositionMs >= m_durationMs) tick();
}

void TimedMediaPlayback::tick()
{
    if (!m_active) return;
    const auto position = positionMs();
    emit positionChanged();
    if (position < m_durationMs) return;

    m_basePositionMs = m_durationMs;
    m_timer.stop();
    m_playing = false;
    m_active = false;
    emit stateChanged();
    emit finished();
}

} // namespace churchpresenter
