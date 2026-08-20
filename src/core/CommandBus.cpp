#include "core/CommandBus.h"

namespace churchpresenter {

CommandBus::CommandBus(QObject *parent)
    : QObject(parent)
{
}

bool CommandBus::registerHandler(const QString &commandType, Handler handler)
{
    const auto normalizedType = commandType.trimmed();
    if (normalizedType.isEmpty() || !handler || m_handlers.contains(normalizedType)) {
        return false;
    }
    m_handlers.insert(normalizedType, std::move(handler));
    return true;
}

CommandResult CommandBus::dispatch(const Command &command)
{
    const auto handler = m_handlers.constFind(command.type);
    if (handler == m_handlers.cend()) {
        return {
            .accepted = false,
            .errorCode = QStringLiteral("unknown_command"),
            .message = QStringLiteral("Comando não registrado: %1").arg(command.type),
            .stateRevision = m_stateRevision,
        };
    }

    auto result = (*handler)(command);
    if (result.accepted) {
        ++m_stateRevision;
    }
    result.stateRevision = m_stateRevision;
    emit commandDispatched(command, result);
    return result;
}

quint64 CommandBus::stateRevision() const
{
    return m_stateRevision;
}

} // namespace churchpresenter
