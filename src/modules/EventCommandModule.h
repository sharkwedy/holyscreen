#pragma once

#include "core/CommandBus.h"
#include "core/EventBus.h"

#include <QObject>

#include <functional>

namespace churchpresenter {

class EventCommandModule final : public QObject {
    Q_OBJECT

public:
    struct Actions {
        std::function<bool(const QString &eventId)> select;
        std::function<bool(const QString &itemId)> executeItem;
        std::function<QVariantMap()> stateSnapshot;
    };

    EventCommandModule(CommandBus &commandBus, EventBus &eventBus, Actions actions,
                       QObject *parent = nullptr);

    CommandResult requestSelect(const QString &eventId,
                                const QString &source = QStringLiteral("operator"));
    CommandResult requestExecuteItem(const QString &itemId,
                                     const QString &source = QStringLiteral("operator"));

private:
    CommandResult dispatch(const QString &type, const QString &key, const QString &value,
                           const QString &source);
    CommandResult execute(const Command &command, const QString &action,
                          const std::function<bool()> &operation);
    CommandBus &m_commandBus;
    EventBus &m_eventBus;
    Actions m_actions;
};

} // namespace churchpresenter
