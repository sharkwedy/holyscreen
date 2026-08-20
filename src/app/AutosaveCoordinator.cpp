#include "app/AutosaveCoordinator.h"

#include <algorithm>

namespace churchpresenter {

AutosaveCoordinator::AutosaveCoordinator(SaveAction saveAction, QObject *parent)
    : QObject(parent)
    , m_saveAction(std::move(saveAction))
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(750);
    connect(&m_timer, &QTimer::timeout, this, [this] { flush(); });
}

void AutosaveCoordinator::setIntervalMs(int intervalMs)
{
    m_timer.setInterval(std::max(1, intervalMs));
}
int AutosaveCoordinator::intervalMs() const { return m_timer.interval(); }
bool AutosaveCoordinator::dirty() const { return m_dirty; }

void AutosaveCoordinator::markDirty()
{
    if (!m_dirty) {
        m_dirty = true;
        emit dirtyChanged(true);
    }
    m_timer.start();
}

bool AutosaveCoordinator::flush()
{
    m_timer.stop();
    if (!m_dirty) return true;
    bool success = false;
    try {
        success = m_saveAction && m_saveAction();
    } catch (...) {
        success = false;
    }
    if (!success) {
        emit saveFailed(QStringLiteral("Não foi possível salvar as alterações automaticamente."));
        return false;
    }
    m_dirty = false;
    emit dirtyChanged(false);
    emit saved();
    return true;
}

void AutosaveCoordinator::discard()
{
    m_timer.stop();
    if (!m_dirty) return;
    m_dirty = false;
    emit dirtyChanged(false);
}

} // namespace churchpresenter
