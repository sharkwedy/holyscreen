#include "app/contexts/EventContext.h"

#include "app/ApplicationController.h"

namespace churchpresenter {

EventContext::EventContext(ApplicationController &controller, QObject *parent)
    : QObject(parent), m_controller(controller)
{
    connect(&controller, &ApplicationController::eventsChanged,
            this, &EventContext::eventsChanged);
    connect(&controller, &ApplicationController::currentEventChanged,
            this, &EventContext::currentEventChanged);
    connect(&controller, &ApplicationController::eventItemsChanged,
            this, &EventContext::eventItemsChanged);
    connect(&controller, &ApplicationController::historyChanged,
            this, &EventContext::historyChanged);
}

QVariantList EventContext::events() const { return m_controller.events(); }
QString EventContext::currentEventId() const { return m_controller.currentEventId(); }
QVariantList EventContext::eventItems() const { return m_controller.eventItems(); }
qint64 EventContext::eventDurationMs() const { return m_controller.eventDurationMs(); }
QVariantList EventContext::history() const { return m_controller.history(); }
QVariantMap EventContext::historyReport() const { return m_controller.historyReport(); }
QString EventContext::createEvent(const QString &title, const QString &scheduledAt) { return m_controller.createEvent(title, scheduledAt); }
void EventContext::selectEvent(const QString &id) { m_controller.selectEvent(id); }
void EventContext::deleteEvent(const QString &id) { m_controller.deleteEvent(id); }
void EventContext::addEventItem(const QString &type, const QString &referenceId,
                                const QString &title, qint64 durationMs)
{
    m_controller.addEventItem(type, referenceId, title, durationMs);
}
void EventContext::removeEventItem(const QString &id) { m_controller.removeEventItem(id); }
void EventContext::moveEventItem(const QString &id, int newIndex) { m_controller.moveEventItem(id, newIndex); }
void EventContext::executeEventItem(const QString &id) { m_controller.executeEventItem(id); }
void EventContext::clearHistory() { m_controller.clearHistory(); }

} // namespace churchpresenter
