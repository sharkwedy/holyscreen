#pragma once

#include "core/CommandBus.h"
#include "core/EventBus.h"
#include "core/UndoManager.h"

#include <QObject>

#include <functional>

namespace churchpresenter {

class MediaCommandModule final : public QObject {
    Q_OBJECT

public:
    struct Actions {
        std::function<bool(const QString &mediaId)> play;
        std::function<bool()> togglePause;
        std::function<bool()> stop;
        std::function<bool(int positionMs)> seek;
        std::function<bool()> previous;
        std::function<bool()> next;
        std::function<bool(const QString &mode)> setRepeat;
        std::function<QVariantMap()> stateSnapshot;
    };

    MediaCommandModule(CommandBus &commandBus, EventBus &eventBus, Actions actions,
                       UndoManager *undoManager = nullptr, QObject *parent = nullptr);

    CommandResult requestPlay(const QString &mediaId,
                              const QString &source = QStringLiteral("operator"));
    CommandResult requestTogglePause(const QString &source = QStringLiteral("operator"));
    CommandResult requestStop(const QString &source = QStringLiteral("operator"));
    CommandResult requestSeek(int positionMs,
                              const QString &source = QStringLiteral("operator"));
    CommandResult requestPrevious(const QString &source = QStringLiteral("operator"));
    CommandResult requestNext(const QString &source = QStringLiteral("operator"));
    CommandResult requestRepeat(const QString &mode,
                                const QString &source = QStringLiteral("operator"));

private:
    CommandResult dispatch(const QString &type, const QVariantMap &payload,
                           const QString &source);
    CommandResult execute(const Command &command, const QString &action,
                          const std::function<bool()> &operation);
    bool applyRepeat(const QString &mode, const QString &correlationId);
    [[nodiscard]] CommandResult invalidPayload(const QString &message) const;

    CommandBus &m_commandBus;
    EventBus &m_eventBus;
    Actions m_actions;
    UndoManager *m_undoManager = nullptr;
};

} // namespace churchpresenter
