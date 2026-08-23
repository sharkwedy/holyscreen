#include "modules/IntegrationCommandModule.h"

#include <QDateTime>
#include <QUuid>

namespace churchpresenter {

IntegrationCommandModule::IntegrationCommandModule(CommandBus &commandBus, EventBus &eventBus,
                                                   Actions actions, QObject *parent)
    : QObject(parent)
    , m_commandBus(commandBus)
    , m_eventBus(eventBus)
    , m_actions(std::move(actions))
{
    m_commandBus.registerHandler(QStringLiteral("integration.test"),
                                 [this](const Command &command) {
        const auto integrationId = command.payload.value(QStringLiteral("integrationId"))
                                       .toString().trimmed();
        if (integrationId.isEmpty()) {
            return invalidPayload(QStringLiteral("integrationId é obrigatório."));
        }
        if (m_actions.definition && m_actions.definition(integrationId).isEmpty()) {
            return invalidPayload(QStringLiteral("Integração não encontrada."));
        }
        if (!m_actions.test) {
            return CommandResult{.accepted = false,
                                 .errorCode = QStringLiteral("operation_failed"),
                                 .message = QStringLiteral("Motor de integrações indisponível.")};
        }
        const auto result = m_actions.test(integrationId, command.id);
        publishResult(integrationId, QStringLiteral("connection.test"), result, command.id);
        return CommandResult{
            .accepted = result.value(QStringLiteral("accepted")).toBool(),
            .errorCode = result.value(QStringLiteral("errorCode")).toString(),
            .message = result.value(QStringLiteral("message")).toString(),
        };
    });

    m_commandBus.registerHandler(QStringLiteral("integration.execute"),
                                 [this](const Command &command) {
        const auto integrationId = command.payload.value(QStringLiteral("integrationId"))
                                       .toString().trimmed();
        const auto operation = command.payload.value(QStringLiteral("operation"))
                                   .toString().trimmed();
        if (integrationId.isEmpty() || operation.isEmpty()) {
            return invalidPayload(QStringLiteral("integrationId e operation são obrigatórios."));
        }
        if (m_actions.definition && m_actions.definition(integrationId).isEmpty()) {
            return invalidPayload(QStringLiteral("Integração não encontrada."));
        }
        if (!m_actions.execute) {
            return CommandResult{.accepted = false,
                                 .errorCode = QStringLiteral("operation_failed"),
                                 .message = QStringLiteral("Motor de integrações indisponível.")};
        }
        const auto result = m_actions.execute(
            integrationId, operation,
            command.payload.value(QStringLiteral("payload")).toMap(), command.id);
        publishResult(integrationId, operation, result, command.id);
        return CommandResult{
            .accepted = result.value(QStringLiteral("accepted")).toBool(),
            .errorCode = result.value(QStringLiteral("errorCode")).toString(),
            .message = result.value(QStringLiteral("message")).toString(),
        };
    });
}

CommandResult IntegrationCommandModule::dispatch(const QString &type, const QVariantMap &payload,
                                                 const QString &source)
{
    return m_commandBus.dispatch(Command{
        .id = QUuid::createUuid().toString(QUuid::WithoutBraces),
        .type = type,
        .payload = payload,
        .source = source,
        .issuedAt = QDateTime::currentDateTimeUtc(),
    });
}

CommandResult IntegrationCommandModule::requestTest(const QString &integrationId,
                                                    const QString &source)
{
    return dispatch(QStringLiteral("integration.test"),
                    {{QStringLiteral("integrationId"), integrationId}}, source);
}

CommandResult IntegrationCommandModule::requestExecute(const QString &integrationId,
                                                       const QString &operation,
                                                       const QVariantMap &payload,
                                                       const QString &source)
{
    return dispatch(QStringLiteral("integration.execute"),
                    {{QStringLiteral("integrationId"), integrationId},
                     {QStringLiteral("operation"), operation},
                     {QStringLiteral("payload"), payload}}, source);
}

void IntegrationCommandModule::publishResult(const QString &integrationId,
                                             const QString &operation,
                                             const QVariantMap &result,
                                             const QString &correlationId)
{
    // O resultado já chega sanitizado pelo motor.
    auto payload = result;
    payload.insert(QStringLiteral("integrationId"), integrationId);
    payload.insert(QStringLiteral("operation"), operation);
    m_eventBus.publish(DomainEvent{
        .type = QStringLiteral("integration.call.finished"),
        .payload = payload,
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .correlationId = correlationId,
    });
}

CommandResult IntegrationCommandModule::invalidPayload(const QString &message) const
{
    return {.accepted = false, .errorCode = QStringLiteral("invalid_payload"), .message = message};
}

} // namespace churchpresenter
