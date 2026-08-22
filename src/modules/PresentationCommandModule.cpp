#include "modules/PresentationCommandModule.h"

#include <QDateTime>
#include <QUuid>

namespace churchpresenter {

PresentationCommandModule::PresentationCommandModule(CommandBus &commandBus,
                                                       EventBus &eventBus,
                                                       Actions actions,
                                                       QObject *parent)
    : QObject(parent)
    , m_commandBus(commandBus)
    , m_eventBus(eventBus)
    , m_actions(std::move(actions))
{
    m_commandBus.registerHandler(QStringLiteral("presentation.slide.show"),
                                 [this](const Command &command) {
        bool valid = false;
        const auto index = command.payload.value(QStringLiteral("index")).toInt(&valid);
        if (!valid || index < 0) {
            return invalidPayload(QStringLiteral("index deve ser um inteiro não negativo."));
        }
        return execute(command, QStringLiteral("slide.show"),
                       [this, index] { return m_actions.show && m_actions.show(index); });
    });
    m_commandBus.registerHandler(QStringLiteral("presentation.slide.next"),
                                 [this](const Command &command) {
        return execute(command, QStringLiteral("slide.next"),
                       [this] { return m_actions.next && m_actions.next(); });
    });
    m_commandBus.registerHandler(QStringLiteral("presentation.slide.previous"),
                                 [this](const Command &command) {
        return execute(command, QStringLiteral("slide.previous"),
                       [this] { return m_actions.previous && m_actions.previous(); });
    });
    m_commandBus.registerHandler(QStringLiteral("presentation.slide.first"),
                                 [this](const Command &command) {
        return execute(command, QStringLiteral("slide.first"),
                       [this] { return m_actions.first && m_actions.first(); });
    });
    m_commandBus.registerHandler(QStringLiteral("presentation.slide.last"),
                                 [this](const Command &command) {
        return execute(command, QStringLiteral("slide.last"),
                       [this] { return m_actions.last && m_actions.last(); });
    });
    m_commandBus.registerHandler(QStringLiteral("presentation.stop"),
                                 [this](const Command &command) {
        return execute(command, QStringLiteral("stop"),
                       [this] { return m_actions.stop && m_actions.stop(); });
    });
}

CommandResult PresentationCommandModule::dispatch(const QString &type, const QVariantMap &payload,
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

CommandResult PresentationCommandModule::requestShow(int index, const QString &source)
{
    return dispatch(QStringLiteral("presentation.slide.show"),
                    {{QStringLiteral("index"), index}}, source);
}
CommandResult PresentationCommandModule::requestNext(const QString &source)
{ return dispatch(QStringLiteral("presentation.slide.next"), {}, source); }
CommandResult PresentationCommandModule::requestPrevious(const QString &source)
{ return dispatch(QStringLiteral("presentation.slide.previous"), {}, source); }
CommandResult PresentationCommandModule::requestFirst(const QString &source)
{ return dispatch(QStringLiteral("presentation.slide.first"), {}, source); }
CommandResult PresentationCommandModule::requestLast(const QString &source)
{ return dispatch(QStringLiteral("presentation.slide.last"), {}, source); }
CommandResult PresentationCommandModule::requestStop(const QString &source)
{ return dispatch(QStringLiteral("presentation.stop"), {}, source); }

CommandResult PresentationCommandModule::execute(const Command &command, const QString &action,
                                                  const std::function<bool()> &operation)
{
    bool succeeded = false;
    try {
        succeeded = operation && operation();
    } catch (...) {
        return {
            .accepted = false,
            .errorCode = QStringLiteral("internal_error"),
            .message = QStringLiteral("Falha interna ao executar o comando de apresentação."),
        };
    }
    if (!succeeded) {
        return {
            .accepted = false,
            .errorCode = QStringLiteral("operation_failed"),
            .message = QStringLiteral("A ação de apresentação não pôde ser executada."),
        };
    }

    auto state = m_actions.stateSnapshot ? m_actions.stateSnapshot() : QVariantMap{};
    state.insert(QStringLiteral("action"), action);
    m_eventBus.publish(DomainEvent{
        .type = QStringLiteral("presentation.state.changed"),
        .payload = state,
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .correlationId = command.id,
    });
    return {
        .accepted = true,
        .message = QStringLiteral("Ação de apresentação executada."),
    };
}

CommandResult PresentationCommandModule::invalidPayload(const QString &message) const
{
    return {.accepted = false, .errorCode = QStringLiteral("invalid_payload"), .message = message};
}

} // namespace churchpresenter
