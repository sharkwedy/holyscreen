#pragma once

#include "core/CommandBus.h"
#include "core/EventBus.h"
#include "core/UndoManager.h"

#include <QObject>
#include <functional>

namespace churchpresenter {

class ThemeCommandModule final : public QObject {
    Q_OBJECT
public:
    struct Actions {
        std::function<QString()> currentThemeId;
        std::function<bool(const QString &themeId)> apply;
        std::function<QVariantMap()> stateSnapshot;
    };

    ThemeCommandModule(CommandBus &commands, EventBus &events, Actions actions,
                       UndoManager *undoManager = nullptr, QObject *parent = nullptr);
    CommandResult requestApply(const QString &themeId,
                               const QString &source = QStringLiteral("operator"));

private:
    bool apply(const QString &themeId, const QString &correlationId);
    CommandBus &m_commands;
    EventBus &m_events;
    Actions m_actions;
    UndoManager *m_undoManager = nullptr;
};

} // namespace churchpresenter
