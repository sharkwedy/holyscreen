#include "modules/MediaCommandModule.h"

#include <QDateTime>
#include <QUuid>

namespace churchpresenter {

MediaCommandModule::MediaCommandModule(CommandBus &commandBus, EventBus &eventBus,
                                       Actions actions, UndoManager *undoManager, QObject *parent)
    : QObject(parent)
    , m_commandBus(commandBus)
    , m_eventBus(eventBus)
    , m_actions(std::move(actions))
    , m_undoManager(undoManager)
{
    m_commandBus.registerHandler(QStringLiteral("media.play"), [this](const Command &command) {
        const auto mediaId = command.payload.value(QStringLiteral("mediaId")).toString().trimmed();
        if (mediaId.isEmpty()) return invalidPayload(QStringLiteral("mediaId é obrigatório."));
        return execute(command, QStringLiteral("play"),
                       [this, mediaId] { return m_actions.play && m_actions.play(mediaId); });
    });
    m_commandBus.registerHandler(QStringLiteral("media.pause.toggle"),
                                 [this](const Command &command) {
        return execute(command, QStringLiteral("pause.toggle"),
                       [this] { return m_actions.togglePause && m_actions.togglePause(); });
    });
    m_commandBus.registerHandler(QStringLiteral("media.stop"), [this](const Command &command) {
        return execute(command, QStringLiteral("stop"),
                       [this] { return m_actions.stop && m_actions.stop(); });
    });
    m_commandBus.registerHandler(QStringLiteral("media.seek"), [this](const Command &command) {
        bool valid = false;
        const auto positionMs = command.payload.value(QStringLiteral("positionMs")).toInt(&valid);
        if (!valid || positionMs < 0) {
            return invalidPayload(QStringLiteral("positionMs deve ser um inteiro não negativo."));
        }
        return execute(command, QStringLiteral("seek"),
                       [this, positionMs] { return m_actions.seek && m_actions.seek(positionMs); });
    });
    m_commandBus.registerHandler(QStringLiteral("media.previous"),
                                 [this](const Command &command) {
        return execute(command, QStringLiteral("previous"),
                       [this] { return m_actions.previous && m_actions.previous(); });
    });
    m_commandBus.registerHandler(QStringLiteral("media.next"), [this](const Command &command) {
        return execute(command, QStringLiteral("next"),
                       [this] { return m_actions.next && m_actions.next(); });
    });
    m_commandBus.registerHandler(QStringLiteral("media.repeat.set"),
                                 [this](const Command &command) {
        const auto mode = command.payload.value(QStringLiteral("mode")).toString();
        if (mode != QStringLiteral("off") && mode != QStringLiteral("one")
            && mode != QStringLiteral("all")) {
            return invalidPayload(QStringLiteral("mode deve ser off, one ou all."));
        }
        const auto previous = m_actions.stateSnapshot
            ? m_actions.stateSnapshot().value(QStringLiteral("repeatMode")).toString()
            : QStringLiteral("off");
        if (!applyRepeat(mode, command.id)) {
            return CommandResult{
                .accepted = false,
                .errorCode = QStringLiteral("operation_failed"),
                .message = QStringLiteral("O modo de repetição não pôde ser atualizado."),
            };
        }
        if (m_undoManager && previous != mode) {
            m_undoManager->record(
                QStringLiteral("Alterar repetição da mídia"),
                [this, previous] {
                    return applyRepeat(previous,
                                       QUuid::createUuid().toString(QUuid::WithoutBraces));
                },
                [this, mode] {
                    return applyRepeat(mode,
                                       QUuid::createUuid().toString(QUuid::WithoutBraces));
                });
        }
        return CommandResult{
            .accepted = true,
            .message = QStringLiteral("Modo de repetição atualizado."),
        };
    });
}

CommandResult MediaCommandModule::dispatch(const QString &type, const QVariantMap &payload,
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

CommandResult MediaCommandModule::requestPlay(const QString &mediaId, const QString &source)
{
    return dispatch(QStringLiteral("media.play"),
                    {{QStringLiteral("mediaId"), mediaId}}, source);
}

CommandResult MediaCommandModule::requestTogglePause(const QString &source)
{
    return dispatch(QStringLiteral("media.pause.toggle"), {}, source);
}

CommandResult MediaCommandModule::requestStop(const QString &source)
{
    return dispatch(QStringLiteral("media.stop"), {}, source);
}

CommandResult MediaCommandModule::requestSeek(int positionMs, const QString &source)
{
    return dispatch(QStringLiteral("media.seek"),
                    {{QStringLiteral("positionMs"), positionMs}}, source);
}

CommandResult MediaCommandModule::requestPrevious(const QString &source)
{
    return dispatch(QStringLiteral("media.previous"), {}, source);
}

CommandResult MediaCommandModule::requestNext(const QString &source)
{
    return dispatch(QStringLiteral("media.next"), {}, source);
}

CommandResult MediaCommandModule::requestRepeat(const QString &mode, const QString &source)
{
    return dispatch(QStringLiteral("media.repeat.set"),
                    {{QStringLiteral("mode"), mode}}, source);
}

bool MediaCommandModule::applyRepeat(const QString &mode, const QString &correlationId)
{
    bool succeeded = false;
    try {
        succeeded = m_actions.setRepeat && m_actions.setRepeat(mode);
    } catch (...) {
        return false;
    }
    if (!succeeded) return false;
    auto state = m_actions.stateSnapshot ? m_actions.stateSnapshot() : QVariantMap{};
    state.insert(QStringLiteral("action"), QStringLiteral("repeat.set"));
    return m_eventBus.publish(DomainEvent{
        .type = QStringLiteral("media.state.changed"),
        .payload = state,
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .correlationId = correlationId,
    });
}

CommandResult MediaCommandModule::execute(const Command &command, const QString &action,
                                          const std::function<bool()> &operation)
{
    bool succeeded = false;
    try {
        succeeded = operation && operation();
    } catch (...) {
        return {
            .accepted = false,
            .errorCode = QStringLiteral("internal_error"),
            .message = QStringLiteral("Falha interna ao executar o comando de mídia."),
        };
    }
    if (!succeeded) {
        return {
            .accepted = false,
            .errorCode = QStringLiteral("operation_failed"),
            .message = QStringLiteral("A ação de mídia não pôde ser executada."),
        };
    }

    auto state = m_actions.stateSnapshot ? m_actions.stateSnapshot() : QVariantMap{};
    state.insert(QStringLiteral("action"), action);
    m_eventBus.publish(DomainEvent{
        .type = QStringLiteral("media.state.changed"),
        .payload = state,
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .correlationId = command.id,
    });
    return {
        .accepted = true,
        .message = QStringLiteral("Ação de mídia executada."),
    };
}

CommandResult MediaCommandModule::invalidPayload(const QString &message) const
{
    return {.accepted = false, .errorCode = QStringLiteral("invalid_payload"), .message = message};
}

} // namespace churchpresenter
