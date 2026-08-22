#pragma once

#include "core/CommandBus.h"
#include "core/EventBus.h"

#include <QObject>

#include <functional>

namespace churchpresenter {

class BibleCommandModule final : public QObject {
    Q_OBJECT

public:
    struct Actions {
        std::function<bool(const QString &reference)> search;
        std::function<bool(int bookId, int chapter, int verse)> present;
        std::function<QVariantMap()> stateSnapshot;
    };

    BibleCommandModule(CommandBus &commandBus, EventBus &eventBus, Actions actions,
                       QObject *parent = nullptr);

    CommandResult requestSearch(const QString &reference,
                                const QString &source = QStringLiteral("operator"));
    CommandResult requestPresent(int bookId, int chapter, int verse,
                                 const QString &source = QStringLiteral("operator"));

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
