#pragma once

#include "core/CommandBus.h"
#include "core/EventBus.h"

#include <QObject>

namespace churchpresenter {

class OutputModule final : public QObject {
    Q_OBJECT

public:
    OutputModule(CommandBus &commandBus, EventBus &eventBus, QObject *parent = nullptr);

    [[nodiscard]] bool blackout() const;
    CommandResult requestBlackout(bool enabled,
                                  const QString &source = QStringLiteral("operator"));

signals:
    void blackoutChanged(bool enabled);

private:
    CommandBus &m_commandBus;
    EventBus &m_eventBus;
    bool m_blackout = false;
};

} // namespace churchpresenter
