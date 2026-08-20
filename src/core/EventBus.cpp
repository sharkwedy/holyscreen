#include "core/EventBus.h"

namespace churchpresenter {

EventBus::EventBus(QObject *parent)
    : QObject(parent)
{
}

bool EventBus::publish(const DomainEvent &event)
{
    if (event.type.trimmed().isEmpty() || !event.occurredAt.isValid()
        || event.correlationId.trimmed().isEmpty()) {
        return false;
    }
    emit eventPublished(event);
    return true;
}

} // namespace churchpresenter
