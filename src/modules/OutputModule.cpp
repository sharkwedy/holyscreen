#include "modules/OutputModule.h"

#include <QDateTime>
#include <QUuid>

namespace churchpresenter {

OutputModule::OutputModule(CommandBus &commandBus, EventBus &eventBus, QObject *parent)
    : QObject(parent)
    , m_commandBus(commandBus)
    , m_eventBus(eventBus)
{
    m_commandBus.registerHandler(QStringLiteral("presentation.blackout.set"),
                                 [this](const Command &command) {
        const auto enabled = command.payload.value(QStringLiteral("enabled")).toBool();
        if (m_blackout != enabled) {
            m_blackout = enabled;
            emit blackoutChanged(enabled);
            m_eventBus.publish(DomainEvent{
                .type = QStringLiteral("presentation.blackout.changed"),
                .payload = {{QStringLiteral("enabled"), enabled}},
                .occurredAt = QDateTime::currentDateTimeUtc(),
                .correlationId = command.id,
            });
        }
        return CommandResult{
            .accepted = true,
            .message = QStringLiteral("Blackout atualizado."),
        };
    });
}

bool OutputModule::blackout() const
{
    return m_blackout;
}

CommandResult OutputModule::requestBlackout(bool enabled, const QString &source)
{
    return m_commandBus.dispatch(Command{
        .id = QUuid::createUuid().toString(QUuid::WithoutBraces),
        .type = QStringLiteral("presentation.blackout.set"),
        .payload = {{QStringLiteral("enabled"), enabled}},
        .source = source,
        .issuedAt = QDateTime::currentDateTimeUtc(),
    });
}

} // namespace churchpresenter
