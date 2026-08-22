#include "modules/StageCommandModule.h"

#include <QDateTime>
#include <QUuid>

namespace churchpresenter {

StageCommandModule::StageCommandModule(CommandBus &commandBus, EventBus &eventBus,
                                       Actions actions, UndoManager *undoManager,
                                       QObject *parent)
    : QObject(parent)
    , m_commandBus(commandBus)
    , m_eventBus(eventBus)
    , m_actions(std::move(actions))
    , m_undoManager(undoManager)
{
    m_commandBus.registerHandler(QStringLiteral("stage.message.set"),
                                 [this](const Command &command) {
        if (!command.payload.contains(QStringLiteral("message"))) {
            return CommandResult{
                .accepted = false,
                .errorCode = QStringLiteral("invalid_payload"),
                .message = QStringLiteral("O campo message é obrigatório."),
            };
        }
        const auto previous = m_actions.message ? m_actions.message() : QString{};
        const auto next = command.payload.value(QStringLiteral("message")).toString().trimmed();
        if (!applyMessage(next, command.id)) {
            return CommandResult{
                .accepted = false,
                .errorCode = QStringLiteral("operation_failed"),
                .message = QStringLiteral("A mensagem do palco não pôde ser atualizada."),
            };
        }
        if (m_undoManager && previous != next) {
            m_undoManager->record(
                QStringLiteral("Alterar mensagem do palco"),
                [this, previous] {
                    return applyMessage(previous,
                                        QUuid::createUuid().toString(QUuid::WithoutBraces));
                },
                [this, next] {
                    return applyMessage(next,
                                        QUuid::createUuid().toString(QUuid::WithoutBraces));
                });
        }
        return CommandResult{
            .accepted = true,
            .message = QStringLiteral("Mensagem do palco atualizada."),
        };
    });
}

CommandResult StageCommandModule::requestMessage(const QString &message, const QString &source)
{
    return m_commandBus.dispatch(Command{
        .id = QUuid::createUuid().toString(QUuid::WithoutBraces),
        .type = QStringLiteral("stage.message.set"),
        .payload = {{QStringLiteral("message"), message}},
        .source = source,
        .issuedAt = QDateTime::currentDateTimeUtc(),
    });
}

bool StageCommandModule::applyMessage(const QString &message, const QString &correlationId)
{
    bool succeeded = false;
    try {
        succeeded = m_actions.setMessage && m_actions.setMessage(message);
    } catch (...) {
        return false;
    }
    if (!succeeded) return false;
    return m_eventBus.publish(DomainEvent{
        .type = QStringLiteral("stage.state.changed"),
        .payload = {{QStringLiteral("message"),
                     m_actions.message ? m_actions.message() : message}},
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .correlationId = correlationId,
    });
}

} // namespace churchpresenter
