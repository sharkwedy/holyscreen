#pragma once

#include "core/CommandBus.h"
#include "core/EventBus.h"
#include "core/UndoManager.h"

#include <QObject>
#include <functional>

namespace churchpresenter {

class PlaylistCommandModule final : public QObject {
    Q_OBJECT
public:
    struct Actions {
        std::function<QVariantList()> snapshot;
        std::function<bool(const QString &id, int index)> move;
        std::function<bool(const QString &id)> remove;
        std::function<bool()> clear;
        std::function<bool(const QVariantList &snapshot)> restore;
    };

    PlaylistCommandModule(CommandBus &commands, EventBus &events, Actions actions,
                          UndoManager *undoManager = nullptr, QObject *parent = nullptr);
    CommandResult requestMove(const QString &id, int index,
                              const QString &source = QStringLiteral("operator"));
    CommandResult requestRemove(const QString &id,
                                const QString &source = QStringLiteral("operator"));
    CommandResult requestClear(const QString &source = QStringLiteral("operator"));

private:
    CommandResult dispatch(const QString &type, const QVariantMap &payload, const QString &source);
    CommandResult change(const Command &command, const QString &label,
                         const std::function<bool()> &operation);
    bool restore(const QVariantList &snapshot, const QString &correlationId);
    bool publish(const QString &action, const QString &correlationId);
    CommandBus &m_commands;
    EventBus &m_events;
    Actions m_actions;
    UndoManager *m_undoManager = nullptr;
};

} // namespace churchpresenter
