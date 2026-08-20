#include "modules/OverlayCommandModule.h"

#include <QDateTime>
#include <QUuid>

namespace churchpresenter {

OverlayCommandModule::OverlayCommandModule(CommandBus &commandBus, EventBus &eventBus,
                                           OverlayController &overlays, QObject *parent)
    : QObject(parent)
    , m_commandBus(commandBus)
    , m_eventBus(eventBus)
    , m_overlays(overlays)
{
    m_commandBus.registerHandler(QStringLiteral("overlay.audience-message.set"),
                                 [this](const Command &command) {
        if (!command.payload.contains(QStringLiteral("message"))) {
            return invalidPayload(QStringLiteral("O campo message é obrigatório."));
        }
        m_overlays.setMessage(command.payload.value(QStringLiteral("message")).toString());
        return complete(command, QStringLiteral("Mensagem da audiência atualizada."));
    });
    m_commandBus.registerHandler(QStringLiteral("overlay.alert.set"),
                                 [this](const Command &command) {
        if (!command.payload.contains(QStringLiteral("message"))) {
            return invalidPayload(QStringLiteral("O campo message é obrigatório."));
        }
        m_overlays.setAlert(command.payload.value(QStringLiteral("message")).toString());
        return complete(command, QStringLiteral("Alerta atualizado."));
    });
    m_commandBus.registerHandler(QStringLiteral("overlay.lower-third.set"),
                                 [this](const Command &command) {
        if (!command.payload.contains(QStringLiteral("title"))
            || !command.payload.contains(QStringLiteral("subtitle"))) {
            return invalidPayload(QStringLiteral("Os campos title e subtitle são obrigatórios."));
        }
        m_overlays.setLowerThird(command.payload.value(QStringLiteral("title")).toString(),
                                 command.payload.value(QStringLiteral("subtitle")).toString());
        return complete(command, QStringLiteral("Lower third atualizado."));
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
    m_eventBus.publish(DomainEvent{
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
        .correlationId = command.id,
    });
    return {.accepted = true, .message = message};
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
