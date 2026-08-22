#pragma once

#include "core/CommandBus.h"
#include "core/EventBus.h"
#include "core/UndoManager.h"

#include <QObject>

#include <functional>

namespace churchpresenter {

class StageCommandModule final : public QObject {
    Q_OBJECT

public:
    struct Actions {
        std::function<QString()> message;
        std::function<bool(const QString &message)> setMessage;
    };

    StageCommandModule(CommandBus &commandBus, EventBus &eventBus, Actions actions,
                       UndoManager *undoManager = nullptr, QObject *parent = nullptr);

    CommandResult requestMessage(const QString &message,
                                 const QString &source = QStringLiteral("operator"));

private:
    bool applyMessage(const QString &message, const QString &correlationId);
    CommandBus &m_commandBus;
    EventBus &m_eventBus;
    Actions m_actions;
    UndoManager *m_undoManager = nullptr;
};

} // namespace churchpresenter
