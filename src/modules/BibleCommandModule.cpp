#include "modules/BibleCommandModule.h"

#include <QDateTime>
#include <QUuid>

namespace churchpresenter {

BibleCommandModule::BibleCommandModule(CommandBus &commandBus, EventBus &eventBus,
                                       Actions actions, QObject *parent)
    : QObject(parent)
    , m_commandBus(commandBus)
    , m_eventBus(eventBus)
    , m_actions(std::move(actions))
{
    m_commandBus.registerHandler(QStringLiteral("bible.search"),
                                 [this](const Command &command) {
        const auto reference = command.payload.value(QStringLiteral("reference"))
                                   .toString().trimmed();
        if (reference.isEmpty()) {
            return invalidPayload(QStringLiteral("reference é obrigatória."));
        }
        return execute(command, QStringLiteral("search"), [this, reference] {
            return m_actions.search && m_actions.search(reference);
        });
    });
    m_commandBus.registerHandler(QStringLiteral("bible.reference.present"),
                                 [this](const Command &command) {
        bool validBook = false;
        bool validChapter = false;
        bool validVerse = false;
        const auto bookId = command.payload.value(QStringLiteral("bookId")).toInt(&validBook);
        const auto chapter = command.payload.value(QStringLiteral("chapter")).toInt(&validChapter);
        const auto verse = command.payload.value(QStringLiteral("verse")).toInt(&validVerse);
        if (!validBook || !validChapter || !validVerse || bookId < 1 || bookId > 66
            || chapter <= 0 || verse <= 0) {
            return invalidPayload(QStringLiteral("bookId, chapter e verse são inválidos."));
        }
        return execute(command, QStringLiteral("reference.present"),
                       [this, bookId, chapter, verse] {
            return m_actions.present && m_actions.present(bookId, chapter, verse);
        });
    });
}

CommandResult BibleCommandModule::dispatch(const QString &type, const QVariantMap &payload,
                                           const QString &source)
{
    return m_commandBus.dispatch(Command{
        .id = QUuid::createUuid().toString(QUuid::WithoutBraces),
        .type = type,
        .payload = payload,
        .source = source,
        .issuedAt = QDateTime::currentDateTimeUtc(),
    });
}

CommandResult BibleCommandModule::requestSearch(const QString &reference, const QString &source)
{
    return dispatch(QStringLiteral("bible.search"),
                    {{QStringLiteral("reference"), reference}}, source);
}

CommandResult BibleCommandModule::requestPresent(int bookId, int chapter, int verse,
                                                 const QString &source)
{
    return dispatch(QStringLiteral("bible.reference.present"),
                    {{QStringLiteral("bookId"), bookId},
                     {QStringLiteral("chapter"), chapter},
                     {QStringLiteral("verse"), verse}}, source);
}

CommandResult BibleCommandModule::execute(const Command &command, const QString &action,
                                          const std::function<bool()> &operation)
{
    bool succeeded = false;
    try {
        succeeded = operation && operation();
    } catch (...) {
        return {.accepted = false, .errorCode = QStringLiteral("internal_error"),
                .message = QStringLiteral("Falha interna no comando bíblico.")};
    }
    if (!succeeded) {
        return {.accepted = false, .errorCode = QStringLiteral("operation_failed"),
                .message = QStringLiteral("A ação bíblica não pôde ser executada.")};
    }
    auto state = m_actions.stateSnapshot ? m_actions.stateSnapshot() : QVariantMap{};
    state.insert(QStringLiteral("action"), action);
    m_eventBus.publish(DomainEvent{
        .type = QStringLiteral("bible.state.changed"),
        .payload = state,
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .correlationId = command.id,
    });
    return {.accepted = true, .message = QStringLiteral("Ação bíblica executada.")};
}

CommandResult BibleCommandModule::invalidPayload(const QString &message) const
{
    return {.accepted = false, .errorCode = QStringLiteral("invalid_payload"), .message = message};
}

} // namespace churchpresenter
