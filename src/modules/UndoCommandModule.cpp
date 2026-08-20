#include "modules/UndoCommandModule.h"

#include <QDateTime>
#include <QUuid>

namespace churchpresenter {

UndoCommandModule::UndoCommandModule(CommandBus &commandBus, EventBus &eventBus,
                                     UndoManager &undoManager, QObject *parent)
    : QObject(parent)
    , m_commandBus(commandBus)
    , m_eventBus(eventBus)
    , m_undoManager(undoManager)
{
    m_commandBus.registerHandler(QStringLiteral("system.undo"),
                                 [this](const Command &command) {
        return execute(command, false);
    });
    m_commandBus.registerHandler(QStringLiteral("system.redo"),
                                 [this](const Command &command) {
        return execute(command, true);
    });
}

CommandResult UndoCommandModule::dispatch(const QString &type, const QString &source)
{
    return m_commandBus.dispatch(Command{
        .id = QUuid::createUuid().toString(QUuid::WithoutBraces),
        .type = type,
        .source = source,
        .issuedAt = QDateTime::currentDateTimeUtc(),
    });
}

CommandResult UndoCommandModule::requestUndo(const QString &source)
{
    return dispatch(QStringLiteral("system.undo"), source);
}

CommandResult UndoCommandModule::requestRedo(const QString &source)
{
    return dispatch(QStringLiteral("system.redo"), source);
}

CommandResult UndoCommandModule::execute(const Command &command, bool redo)
{
    const auto operation = redo ? m_undoManager.redo() : m_undoManager.undo();
    if (!operation.success) {
        return {
            .accepted = false,
            .errorCode = QStringLiteral("operation_unavailable"),
            .message = operation.error,
        };
    }

    m_eventBus.publish(DomainEvent{
        .type = QStringLiteral("system.undo-state.changed"),
        .payload = {
            {QStringLiteral("action"), redo ? QStringLiteral("redo") : QStringLiteral("undo")},
            {QStringLiteral("label"), operation.label},
            {QStringLiteral("canUndo"), m_undoManager.canUndo()},
            {QStringLiteral("canRedo"), m_undoManager.canRedo()},
            {QStringLiteral("undoLabel"), m_undoManager.undoLabel()},
            {QStringLiteral("redoLabel"), m_undoManager.redoLabel()},
        },
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .correlationId = command.id,
    });
    return {
        .accepted = true,
        .message = redo ? QStringLiteral("Ação refeita.") : QStringLiteral("Ação desfeita."),
    };
}

} // namespace churchpresenter
