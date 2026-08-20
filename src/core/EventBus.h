#pragma once

#include "core/EventTypes.h"

#include <QObject>

namespace churchpresenter {

class EventBus final : public QObject {
    Q_OBJECT

public:
    explicit EventBus(QObject *parent = nullptr);

    bool publish(const DomainEvent &event);

signals:
    void eventPublished(const churchpresenter::DomainEvent &event);
};

} // namespace churchpresenter
