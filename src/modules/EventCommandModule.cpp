#include "modules/EventCommandModule.h"

#include <QDateTime>
#include <QUuid>

namespace churchpresenter {

EventCommandModule::EventCommandModule(CommandBus &commandBus, EventBus &eventBus,
                                       Actions actions, QObject *parent)
    : QObject(parent)
    , m_commandBus(commandBus)
    , m_eventBus(eventBus)
    , m_actions(std::move(actions))
{
    m_commandBus.registerHandler(QStringLiteral("event.select"),
                                 [this](const Command &command) {
        const auto eventId = command.payload.value(QStringLiteral("eventId"))
                                 .toString().trimmed();
        if (eventId.isEmpty()) {
            return CommandResult{.accepted = false,
                                 .errorCode = QStringLiteral("invalid_payload"),
                                 .message = QStringLiteral("eventId é obrigatório.")};
        }
        return execute(command, QStringLiteral("select"), [this, eventId] {
            return m_actions.select && m_actions.select(eventId);
        });
    });
    m_commandBus.registerHandler(QStringLiteral("event.item.execute"),
                                 [this](const Command &command) {
        const auto itemId = command.payload.value(QStringLiteral("itemId"))
                                .toString().trimmed();
        if (itemId.isEmpty()) {
            return CommandResult{.accepted = false,
                                 .errorCode = QStringLiteral("invalid_payload"),
                                 .message = QStringLiteral("itemId é obrigatório.")};
        }
        return execute(command, QStringLiteral("item.execute"), [this, itemId] {
            return m_actions.executeItem && m_actions.executeItem(itemId);
        });
    });
}

CommandResult EventCommandModule::dispatch(const QString &type, const QString &key,
                                           const QString &value, const QString &source)
{
    return m_commandBus.dispatch(Command{
        .id = QUuid::createUuid().toString(QUuid::WithoutBraces),
        .type = type,
        .payload = {{key, value}},
        .source = source,
        .issuedAt = QDateTime::currentDateTimeUtc(),
    });
}

CommandResult EventCommandModule::requestSelect(const QString &eventId, const QString &source)
{
    return dispatch(QStringLiteral("event.select"), QStringLiteral("eventId"), eventId, source);
}

CommandResult EventCommandModule::requestExecuteItem(const QString &itemId,
                                                     const QString &source)
{
    return dispatch(QStringLiteral("event.item.execute"), QStringLiteral("itemId"), itemId,
                    source);
}

CommandResult EventCommandModule::execute(const Command &command, const QString &action,
                                          const std::function<bool()> &operation)
{
    bool succeeded = false;
    try {
        succeeded = operation && operation();
    } catch (...) {
        return {.accepted = false, .errorCode = QStringLiteral("internal_error"),
                .message = QStringLiteral("Falha interna no comando de evento.")};
    }
    if (!succeeded) {
        return {.accepted = false, .errorCode = QStringLiteral("operation_failed"),
                .message = QStringLiteral("A ação do evento não pôde ser executada.")};
    }
    auto state = m_actions.stateSnapshot ? m_actions.stateSnapshot() : QVariantMap{};
    state.insert(QStringLiteral("action"), action);
    m_eventBus.publish(DomainEvent{
        .type = QStringLiteral("event.state.changed"),
        .payload = state,
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .correlationId = command.id,
    });
    return {.accepted = true, .message = QStringLiteral("Ação do evento executada.")};
}

} // namespace churchpresenter
