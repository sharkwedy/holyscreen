#include "modules/ThemeCommandModule.h"

#include <QDateTime>
#include <QUuid>

namespace churchpresenter {

ThemeCommandModule::ThemeCommandModule(CommandBus &commands, EventBus &events, Actions actions,
                                       UndoManager *undoManager, QObject *parent)
    : QObject(parent), m_commands(commands), m_events(events), m_actions(std::move(actions)),
      m_undoManager(undoManager)
{
    m_commands.registerHandler(QStringLiteral("settings.theme.apply"),
                               [this](const Command &command) {
        const auto themeId = command.payload.value(QStringLiteral("themeId")).toString().trimmed();
        if (themeId.isEmpty()) return CommandResult{.accepted=false,.errorCode=QStringLiteral("invalid_payload"),.message=QStringLiteral("themeId é obrigatório.")};
        const auto previous = m_actions.currentThemeId ? m_actions.currentThemeId() : QString{};
        if (!apply(themeId, command.id)) return CommandResult{.accepted=false,.errorCode=QStringLiteral("operation_failed"),.message=QStringLiteral("O tema não pôde ser aplicado.")};
        if (m_undoManager && !previous.isEmpty() && previous != themeId) {
            m_undoManager->record(QStringLiteral("Aplicar tema"),
                [this, previous]{return apply(previous, QUuid::createUuid().toString(QUuid::WithoutBraces));},
                [this, themeId]{return apply(themeId, QUuid::createUuid().toString(QUuid::WithoutBraces));});
        }
        return CommandResult{.accepted=true,.message=QStringLiteral("Tema aplicado.")};
    });
}

CommandResult ThemeCommandModule::requestApply(const QString &themeId, const QString &source)
{
    return m_commands.dispatch(Command{.id=QUuid::createUuid().toString(QUuid::WithoutBraces),
        .type=QStringLiteral("settings.theme.apply"),.payload={{QStringLiteral("themeId"),themeId}},
        .source=source,.issuedAt=QDateTime::currentDateTimeUtc()});
}

bool ThemeCommandModule::apply(const QString &themeId, const QString &correlationId)
{
    if (!m_actions.apply || !m_actions.apply(themeId)) return false;
    auto state=m_actions.stateSnapshot?m_actions.stateSnapshot():QVariantMap{};
    m_events.publish(DomainEvent{.type=QStringLiteral("settings.theme.changed"),.payload=state,
        .occurredAt=QDateTime::currentDateTimeUtc(),.correlationId=correlationId});
    return true;
}

} // namespace churchpresenter
