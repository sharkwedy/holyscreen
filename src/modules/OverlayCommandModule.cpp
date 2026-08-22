#include "modules/OverlayCommandModule.h"

#include <QDateTime>
#include <QUuid>

namespace churchpresenter {

OverlayCommandModule::OverlayCommandModule(CommandBus &commandBus, EventBus &eventBus,
                                           OverlayController &overlays, UndoManager *undoManager,
                                           QObject *parent)
    : QObject(parent)
    , m_commandBus(commandBus)
    , m_eventBus(eventBus)
    , m_overlays(overlays)
    , m_undoManager(undoManager)
{
    m_commandBus.registerHandler(QStringLiteral("overlay.audience-message.set"),
                                 [this](const Command &command) {
        if (!command.payload.contains(QStringLiteral("message"))) {
            return invalidPayload(QStringLiteral("O campo message é obrigatório."));
        }
        const auto previous = m_overlays.message();
        const auto next = command.payload.value(QStringLiteral("message")).toString().trimmed();
        m_overlays.setMessage(next);
        const auto result = complete(command, QStringLiteral("Mensagem da audiência atualizada."));
        if (m_undoManager && previous != m_overlays.message()) {
            m_undoManager->record(
                QStringLiteral("Alterar mensagem da audiência"),
                [this, previous] {
                    m_overlays.setMessage(previous);
                    return publishState(QUuid::createUuid().toString(QUuid::WithoutBraces));
                },
                [this, next] {
                    m_overlays.setMessage(next);
                    return publishState(QUuid::createUuid().toString(QUuid::WithoutBraces));
                });
        }
        return result;
    });
    m_commandBus.registerHandler(QStringLiteral("overlay.alert.set"),
                                 [this](const Command &command) {
        if (!command.payload.contains(QStringLiteral("message"))) {
            return invalidPayload(QStringLiteral("O campo message é obrigatório."));
        }
        const auto previous = m_overlays.alert();
        const auto next = command.payload.value(QStringLiteral("message")).toString().trimmed();
        m_overlays.setAlert(next);
        const auto result = complete(command, QStringLiteral("Alerta atualizado."));
        if (m_undoManager && previous != m_overlays.alert()) {
            m_undoManager->record(
                QStringLiteral("Alterar alerta"),
                [this, previous] {
                    m_overlays.setAlert(previous);
                    return publishState(QUuid::createUuid().toString(QUuid::WithoutBraces));
                },
                [this, next] {
                    m_overlays.setAlert(next);
                    return publishState(QUuid::createUuid().toString(QUuid::WithoutBraces));
                });
        }
        return result;
    });
    m_commandBus.registerHandler(QStringLiteral("overlay.lower-third.set"),
                                 [this](const Command &command) {
        if (!command.payload.contains(QStringLiteral("title"))
            || !command.payload.contains(QStringLiteral("subtitle"))) {
            return invalidPayload(QStringLiteral("Os campos title e subtitle são obrigatórios."));
        }
        const auto previousTitle = m_overlays.lowerThirdTitle();
        const auto previousSubtitle = m_overlays.lowerThirdSubtitle();
        const auto title = command.payload.value(QStringLiteral("title")).toString().trimmed();
        const auto subtitle = command.payload.value(QStringLiteral("subtitle")).toString().trimmed();
        m_overlays.setLowerThird(title, subtitle);
        const auto result = complete(command, QStringLiteral("Lower third atualizado."));
        if (m_undoManager
            && (previousTitle != m_overlays.lowerThirdTitle()
                || previousSubtitle != m_overlays.lowerThirdSubtitle())) {
            m_undoManager->record(
                QStringLiteral("Alterar lower third"),
                [this, previousTitle, previousSubtitle] {
                    m_overlays.setLowerThird(previousTitle, previousSubtitle);
                    return publishState(QUuid::createUuid().toString(QUuid::WithoutBraces));
                },
                [this, title, subtitle] {
                    m_overlays.setLowerThird(title, subtitle);
                    return publishState(QUuid::createUuid().toString(QUuid::WithoutBraces));
                });
        }
        return result;
    });
    m_commandBus.registerHandler(QStringLiteral("timer.countdown.start"),
                                 [this](const Command &command) {
        bool valid = false;
        const auto seconds = command.payload.value(QStringLiteral("seconds")).toInt(&valid);
        if (!valid || seconds <= 0) {
            return invalidPayload(QStringLiteral("seconds deve ser um inteiro positivo."));
        }
        m_overlays.startCountdown(seconds);
        return complete(command, QStringLiteral("Contagem regressiva iniciada."));
    });
    m_commandBus.registerHandler(QStringLiteral("timer.countdown.stop"),
                                 [this](const Command &command) {
        m_overlays.stopCountdown();
        return complete(command, QStringLiteral("Contagem regressiva interrompida."));
    });
    m_commandBus.registerHandler(QStringLiteral("timer.stopwatch.start"),
                                 [this](const Command &command) {
        m_overlays.startStopwatch();
        return complete(command, QStringLiteral("Cronômetro iniciado."));
    });
    m_commandBus.registerHandler(QStringLiteral("timer.stopwatch.pause"),
                                 [this](const Command &command) {
        m_overlays.pauseStopwatch();
        return complete(command, QStringLiteral("Cronômetro pausado."));
    });
    m_commandBus.registerHandler(QStringLiteral("timer.stopwatch.reset"),
                                 [this](const Command &command) {
        m_overlays.resetStopwatch();
        return complete(command, QStringLiteral("Cronômetro zerado."));
    });
}

CommandResult OverlayCommandModule::complete(const Command &command, const QString &message)
{
    publishState(command.id);
    return {.accepted = true, .message = message};
}

bool OverlayCommandModule::publishState(const QString &correlationId)
{
    return m_eventBus.publish(DomainEvent{
        .type = QStringLiteral("overlay.state.changed"),
        .payload = {
            {QStringLiteral("audienceMessage"), m_overlays.message()},
            {QStringLiteral("alertMessage"), m_overlays.alert()},
            {QStringLiteral("lowerThirdTitle"), m_overlays.lowerThirdTitle()},
            {QStringLiteral("lowerThirdSubtitle"), m_overlays.lowerThirdSubtitle()},
            {QStringLiteral("countdownText"), m_overlays.countdownText()},
            {QStringLiteral("countdownRunning"), m_overlays.countdownRunning()},
            {QStringLiteral("stopwatchText"), m_overlays.stopwatchText()},
            {QStringLiteral("stopwatchRunning"), m_overlays.stopwatchRunning()},
        },
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .correlationId = correlationId,
    });
}

CommandResult OverlayCommandModule::invalidPayload(const QString &message) const
{
    return {
        .accepted = false,
        .errorCode = QStringLiteral("invalid_payload"),
        .message = message,
    };
}

CommandResult OverlayCommandModule::dispatch(const QString &type, const QVariantMap &payload,
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

CommandResult OverlayCommandModule::requestAudienceMessage(const QString &message,
                                                            const QString &source)
{
    return dispatch(QStringLiteral("overlay.audience-message.set"),
                    {{QStringLiteral("message"), message}}, source);
}

CommandResult OverlayCommandModule::requestAlert(const QString &message, const QString &source)
{
    return dispatch(QStringLiteral("overlay.alert.set"),
                    {{QStringLiteral("message"), message}}, source);
}

CommandResult OverlayCommandModule::requestLowerThird(const QString &title, const QString &subtitle,
                                                       const QString &source)
{
    return dispatch(QStringLiteral("overlay.lower-third.set"),
                    {{QStringLiteral("title"), title}, {QStringLiteral("subtitle"), subtitle}},
                    source);
}

CommandResult OverlayCommandModule::requestCountdownStart(int seconds, const QString &source)
{
    return dispatch(QStringLiteral("timer.countdown.start"),
                    {{QStringLiteral("seconds"), seconds}}, source);
}

CommandResult OverlayCommandModule::requestCountdownStop(const QString &source)
{
    return dispatch(QStringLiteral("timer.countdown.stop"), {}, source);
}

CommandResult OverlayCommandModule::requestStopwatchStart(const QString &source)
{
    return dispatch(QStringLiteral("timer.stopwatch.start"), {}, source);
}

CommandResult OverlayCommandModule::requestStopwatchPause(const QString &source)
{
    return dispatch(QStringLiteral("timer.stopwatch.pause"), {}, source);
}

CommandResult OverlayCommandModule::requestStopwatchReset(const QString &source)
{
    return dispatch(QStringLiteral("timer.stopwatch.reset"), {}, source);
}

} // namespace churchpresenter
