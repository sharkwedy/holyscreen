#pragma once

#include "core/CommandBus.h"
#include "core/EventBus.h"
#include "core/UndoManager.h"

#include <QObject>

namespace churchpresenter {

class OutputModule final : public QObject {
    Q_OBJECT

public:
    OutputModule(CommandBus &commandBus, EventBus &eventBus,
                 UndoManager *undoManager = nullptr, QObject *parent = nullptr);

    [[nodiscard]] bool blackout() const;
    CommandResult requestBlackout(bool enabled,
                                  const QString &source = QStringLiteral("operator"));

signals:
    void blackoutChanged(bool enabled);

private:
    bool applyBlackout(bool enabled, const QString &correlationId);

    CommandBus &m_commandBus;
    EventBus &m_eventBus;
    UndoManager *m_undoManager = nullptr;
    bool m_blackout = false;
};

} // namespace churchpresenter
