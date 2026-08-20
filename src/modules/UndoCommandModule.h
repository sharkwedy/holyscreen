#pragma once

#include "core/CommandBus.h"
#include "core/EventBus.h"
#include "core/UndoManager.h"

#include <QObject>

namespace churchpresenter {

class UndoCommandModule final : public QObject {
    Q_OBJECT

public:
    UndoCommandModule(CommandBus &commandBus, EventBus &eventBus, UndoManager &undoManager,
                      QObject *parent = nullptr);

    CommandResult requestUndo(const QString &source = QStringLiteral("operator"));
    CommandResult requestRedo(const QString &source = QStringLiteral("operator"));

private:
    CommandResult dispatch(const QString &type, const QString &source);
    CommandResult execute(const Command &command, bool redo);

    CommandBus &m_commandBus;
    EventBus &m_eventBus;
    UndoManager &m_undoManager;
};

} // namespace churchpresenter
