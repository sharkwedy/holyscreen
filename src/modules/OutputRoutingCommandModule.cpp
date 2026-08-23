#include "modules/OutputRoutingCommandModule.h"

#include "screens/OutputRole.h"

#include <QDateTime>
#include <QMetaType>
#include <QUuid>

namespace churchpresenter {

OutputRoutingCommandModule::OutputRoutingCommandModule(CommandBus &commandBus,
                                                       EventBus &eventBus, Actions actions,
                                                       UndoManager *undoManager, QObject *parent)
    : QObject(parent)
    , m_commandBus(commandBus)
    , m_eventBus(eventBus)
    , m_actions(std::move(actions))
    , m_undoManager(undoManager)
{
    m_commandBus.registerHandler(QStringLiteral("output.enabled.set"),
                                 [this](const Command &command) {
        const auto fingerprint = command.payload.value(QStringLiteral("fingerprint"))
                                     .toString().trimmed();
        const auto enabledValue = command.payload.value(QStringLiteral("enabled"));
        if (fingerprint.isEmpty() || enabledValue.metaType().id() != QMetaType::Bool) {
            return invalidPayload(QStringLiteral("fingerprint e enabled booleano são obrigatórios."));
        }
        const auto previousOutput = m_actions.output ? m_actions.output(fingerprint) : QVariantMap{};
        if (previousOutput.isEmpty()) {
            return invalidPayload(QStringLiteral("Saída não encontrada."));
        }
        const auto previous = previousOutput.value(QStringLiteral("enabled")).toBool();
        const auto enabled = enabledValue.toBool();
        if (!applyEnabled(fingerprint, enabled, command.id)) {
            return CommandResult{.accepted = false,
                                 .errorCode = QStringLiteral("operation_failed"),
                                 .message = QStringLiteral("A saída não pôde ser atualizada.")};
        }
        if (m_undoManager && previous != enabled) {
            m_undoManager->record(
                QStringLiteral("Alterar ativação da saída"),
                [this, fingerprint, previous] {
                    return applyEnabled(fingerprint, previous,
                        QUuid::createUuid().toString(QUuid::WithoutBraces));
                },
                [this, fingerprint, enabled] {
                    return applyEnabled(fingerprint, enabled,
                        QUuid::createUuid().toString(QUuid::WithoutBraces));
                });
        }
        return CommandResult{.accepted = true, .message = QStringLiteral("Saída atualizada.")};
    });
    m_commandBus.registerHandler(QStringLiteral("output.role.set"),
                                 [this](const Command &command) {
        const auto fingerprint = command.payload.value(QStringLiteral("fingerprint"))
                                     .toString().trimmed();
        const auto role = command.payload.value(QStringLiteral("role")).toString().trimmed();
        if (fingerprint.isEmpty() || !isOutputRoleName(role)) {
            return invalidPayload(QStringLiteral("fingerprint e role %1 são obrigatórios.")
                                      .arg(outputRoleNames().join(QLatin1Char('/'))));
        }
        const auto previousOutput = m_actions.output ? m_actions.output(fingerprint) : QVariantMap{};
        if (previousOutput.isEmpty()) return invalidPayload(QStringLiteral("Saída não encontrada."));
        const auto previous = previousOutput.value(QStringLiteral("role")).toString();
        if (!applyRole(fingerprint, role, command.id)) {
            return CommandResult{.accepted = false,
                                 .errorCode = QStringLiteral("operation_failed"),
                                 .message = QStringLiteral("O papel da saída não pôde ser atualizado.")};
        }
        if (m_undoManager && previous != role) {
            m_undoManager->record(
                QStringLiteral("Alterar papel da saída"),
                [this, fingerprint, previous] {
                    return applyRole(fingerprint, previous,
                        QUuid::createUuid().toString(QUuid::WithoutBraces));
                },
                [this, fingerprint, role] {
                    return applyRole(fingerprint, role,
                        QUuid::createUuid().toString(QUuid::WithoutBraces));
                });
        }
        return CommandResult{.accepted = true, .message = QStringLiteral("Papel da saída atualizado.")};
    });
    m_commandBus.registerHandler(QStringLiteral("output.media-enabled.set"),
                                 [this](const Command &command) {
        const auto fingerprint = command.payload.value(QStringLiteral("fingerprint"))
                                     .toString().trimmed();
        const auto enabledValue = command.payload.value(QStringLiteral("enabled"));
        if (fingerprint.isEmpty() || enabledValue.metaType().id() != QMetaType::Bool) {
            return invalidPayload(QStringLiteral("fingerprint e enabled booleano são obrigatórios."));
        }
        const auto previousOutput = m_actions.output ? m_actions.output(fingerprint) : QVariantMap{};
        if (previousOutput.isEmpty()) return invalidPayload(QStringLiteral("Saída não encontrada."));
        const auto previous = previousOutput.value(QStringLiteral("mediaEnabled")).toBool();
        const auto enabled = enabledValue.toBool();
        if (!applyMediaEnabled(fingerprint, enabled, command.id)) {
            return CommandResult{.accepted = false,
                                 .errorCode = QStringLiteral("operation_failed"),
                                 .message = QStringLiteral("A mídia da saída não pôde ser atualizada.")};
        }
        if (m_undoManager && previous != enabled) {
            m_undoManager->record(
                QStringLiteral("Alterar mídia da saída"),
                [this, fingerprint, previous] {
                    return applyMediaEnabled(fingerprint, previous,
                        QUuid::createUuid().toString(QUuid::WithoutBraces));
                },
                [this, fingerprint, enabled] {
                    return applyMediaEnabled(fingerprint, enabled,
                        QUuid::createUuid().toString(QUuid::WithoutBraces));
                });
        }
        return CommandResult{.accepted = true, .message = QStringLiteral("Mídia da saída atualizada.")};
    });
}

CommandResult OutputRoutingCommandModule::dispatch(const QString &type, const QVariantMap &payload,
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

CommandResult OutputRoutingCommandModule::requestEnabled(const QString &fingerprint, bool enabled,
                                                         const QString &source)
{
    return dispatch(QStringLiteral("output.enabled.set"),
                    {{QStringLiteral("fingerprint"), fingerprint},
                     {QStringLiteral("enabled"), enabled}}, source);
}

CommandResult OutputRoutingCommandModule::requestRole(const QString &fingerprint,
                                                      const QString &role,
                                                      const QString &source)
{
    return dispatch(QStringLiteral("output.role.set"),
                    {{QStringLiteral("fingerprint"), fingerprint},
                     {QStringLiteral("role"), role}}, source);
}

CommandResult OutputRoutingCommandModule::requestMediaEnabled(const QString &fingerprint,
                                                              bool enabled,
                                                              const QString &source)
{
    return dispatch(QStringLiteral("output.media-enabled.set"),
                    {{QStringLiteral("fingerprint"), fingerprint},
                     {QStringLiteral("enabled"), enabled}}, source);
}

bool OutputRoutingCommandModule::applyEnabled(const QString &fingerprint, bool enabled,
                                              const QString &correlationId)
{
    if (!m_actions.setEnabled || !m_actions.setEnabled(fingerprint, enabled)) return false;
    return publishState(fingerprint, QStringLiteral("enabled.set"), correlationId);
}

bool OutputRoutingCommandModule::applyRole(const QString &fingerprint, const QString &role,
                                           const QString &correlationId)
{
    if (!m_actions.setRole || !m_actions.setRole(fingerprint, role)) return false;
    return publishState(fingerprint, QStringLiteral("role.set"), correlationId);
}

bool OutputRoutingCommandModule::applyMediaEnabled(const QString &fingerprint, bool enabled,
                                                   const QString &correlationId)
{
    if (!m_actions.setMediaEnabled || !m_actions.setMediaEnabled(fingerprint, enabled)) return false;
    return publishState(fingerprint, QStringLiteral("media-enabled.set"), correlationId);
}

bool OutputRoutingCommandModule::publishState(const QString &fingerprint, const QString &action,
                                              const QString &correlationId)
{
    auto state = m_actions.output ? m_actions.output(fingerprint) : QVariantMap{};
    state.insert(QStringLiteral("action"), action);
    return m_eventBus.publish(DomainEvent{
        .type = QStringLiteral("output.routing.changed"),
        .payload = state,
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .correlationId = correlationId,
    });
}

CommandResult OutputRoutingCommandModule::invalidPayload(const QString &message) const
{
    return {.accepted = false, .errorCode = QStringLiteral("invalid_payload"), .message = message};
}

} // namespace churchpresenter
