#pragma once

#include "core/CommandBus.h"
#include "core/EventBus.h"

#include <QObject>

#include <functional>

namespace churchpresenter {

class PresentationCommandModule final : public QObject {
    Q_OBJECT

public:
    struct Actions {
        std::function<bool(int index)> show;
        std::function<bool()> next;
        std::function<bool()> previous;
        std::function<bool()> first;
        std::function<bool()> last;
        std::function<bool()> stop;
        std::function<QVariantMap()> stateSnapshot;
    };

    PresentationCommandModule(CommandBus &commandBus, EventBus &eventBus,
                              Actions actions, QObject *parent = nullptr);

    CommandResult requestShow(int index, const QString &source = QStringLiteral("operator"));
    CommandResult requestNext(const QString &source = QStringLiteral("operator"));
    CommandResult requestPrevious(const QString &source = QStringLiteral("operator"));
    CommandResult requestFirst(const QString &source = QStringLiteral("operator"));
    CommandResult requestLast(const QString &source = QStringLiteral("operator"));
    CommandResult requestStop(const QString &source = QStringLiteral("operator"));

private:
    CommandResult dispatch(const QString &type, const QVariantMap &payload,
                           const QString &source);
    CommandResult execute(const Command &command, const QString &action,
                          const std::function<bool()> &operation);
    [[nodiscard]] CommandResult invalidPayload(const QString &message) const;
    CommandBus &m_commandBus;
    EventBus &m_eventBus;
    Actions m_actions;
};

} // namespace churchpresenter
