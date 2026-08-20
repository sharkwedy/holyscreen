#include "modules/OutputModule.h"

#include <QDateTime>
#include <QUuid>

namespace churchpresenter {

OutputModule::OutputModule(CommandBus &commandBus, EventBus &eventBus,
                           UndoManager *undoManager, QObject *parent)
    : QObject(parent)
    , m_commandBus(commandBus)
    , m_eventBus(eventBus)
    , m_undoManager(undoManager)
{
    m_commandBus.registerHandler(QStringLiteral("presentation.blackout.set"),
                                 [this](const Command &command) {
        const auto enabledValue = command.payload.value(QStringLiteral("enabled"));
        if (!command.payload.contains(QStringLiteral("enabled"))
            || enabledValue.metaType().id() != QMetaType::Bool) {
            return CommandResult{
                .accepted = false,
                .errorCode = QStringLiteral("invalid_payload"),
                .message = QStringLiteral("enabled deve ser booleano."),
            };
        }
        const auto enabled = enabledValue.toBool();
        const auto previous = m_blackout;
        if (previous != enabled) {
            applyBlackout(enabled, command.id);
            if (m_undoManager) {
                const auto label = enabled ? QStringLiteral("Ativar blackout")
                                           : QStringLiteral("Desativar blackout");
                m_undoManager->record(
                    label,
                    [this, previous] {
                        return applyBlackout(
                            previous, QUuid::createUuid().toString(QUuid::WithoutBraces));
                    },
                    [this, enabled] {
                        return applyBlackout(
                            enabled, QUuid::createUuid().toString(QUuid::WithoutBraces));
                    });
            }
        }
        return CommandResult{
            .accepted = true,
            .message = QStringLiteral("Blackout atualizado."),
        };
    });
}

bool OutputModule::applyBlackout(bool enabled, const QString &correlationId)
{
    if (m_blackout == enabled) return true;
    m_blackout = enabled;
    emit blackoutChanged(enabled);
    return m_eventBus.publish(DomainEvent{
        .type = QStringLiteral("presentation.blackout.changed"),
        .payload = {{QStringLiteral("enabled"), enabled}},
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .correlationId = correlationId,
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
