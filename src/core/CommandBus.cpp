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

bool CommandBus::hasHandler(const QString &commandType) const
{
    return m_handlers.contains(commandType.trimmed());
}

QStringList CommandBus::registeredCommandTypes() const
{
    auto types = m_handlers.keys();
    types.sort();
    return types;
}

CommandResult CommandBus::dispatch(const Command &command)
{
    if (command.id.trimmed().isEmpty() || command.type.trimmed().isEmpty()
        || command.source.trimmed().isEmpty() || !command.issuedAt.isValid()) {
        const CommandResult result{
            .accepted = false,
            .errorCode = QStringLiteral("invalid_command"),
            .message = QStringLiteral("Comando sem id, tipo, origem ou data válida."),
            .stateRevision = m_stateRevision,
        };
        emit commandDispatched(command, result);
        return result;
    }

    const auto handler = m_handlers.constFind(command.type);
    if (handler == m_handlers.cend()) {
        const CommandResult result{
            .accepted = false,
            .errorCode = QStringLiteral("unknown_command"),
            .message = QStringLiteral("Comando não registrado: %1").arg(command.type),
            .stateRevision = m_stateRevision,
        };
        emit commandDispatched(command, result);
        return result;
    }

    CommandResult result;
    try {
        result = (*handler)(command);
    } catch (...) {
        result = {
            .accepted = false,
            .errorCode = QStringLiteral("internal_error"),
            .message = QStringLiteral("Falha interna ao processar o comando."),
            .stateRevision = m_stateRevision,
        };
        emit commandDispatched(command, result);
        return result;
    }
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
