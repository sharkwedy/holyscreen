#pragma once

#include "core/CommandTypes.h"

#include <QHash>
#include <QObject>

#include <functional>

namespace churchpresenter {

class CommandBus final : public QObject {
    Q_OBJECT

public:
    using Handler = std::function<CommandResult(const Command &)>;

    explicit CommandBus(QObject *parent = nullptr);

    bool registerHandler(const QString &commandType, Handler handler);
    [[nodiscard]] bool hasHandler(const QString &commandType) const;
    [[nodiscard]] QStringList registeredCommandTypes() const;
    CommandResult dispatch(const Command &command);
    [[nodiscard]] quint64 stateRevision() const;

signals:
    void commandDispatched(const churchpresenter::Command &command,
                           const churchpresenter::CommandResult &result);

private:
    QHash<QString, Handler> m_handlers;
    quint64 m_stateRevision = 0;
};

} // namespace churchpresenter
