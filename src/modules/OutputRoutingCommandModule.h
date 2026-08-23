#pragma once

#include "core/CommandBus.h"
#include "core/EventBus.h"
#include "core/UndoManager.h"

#include <QObject>
#include <QVariantMap>

#include <functional>

namespace churchpresenter {

class OutputRoutingCommandModule final : public QObject {
    Q_OBJECT

public:
    struct Actions {
        std::function<QVariantMap(const QString &fingerprint)> output;
        std::function<bool(const QString &fingerprint, bool enabled)> setEnabled;
        std::function<bool(const QString &fingerprint, const QString &role)> setRole;
        std::function<bool(const QString &fingerprint, bool enabled)> setMediaEnabled;
        std::function<QVariantMap(const QString &fingerprint)> broadcastProfile;
        std::function<bool(const QString &fingerprint, const QVariantMap &profile)>
            setBroadcastProfile;
    };

    OutputRoutingCommandModule(CommandBus &commandBus, EventBus &eventBus, Actions actions,
                               UndoManager *undoManager = nullptr, QObject *parent = nullptr);

    CommandResult requestEnabled(const QString &fingerprint, bool enabled,
                                 const QString &source = QStringLiteral("operator"));
    CommandResult requestRole(const QString &fingerprint, const QString &role,
                              const QString &source = QStringLiteral("operator"));
    CommandResult requestMediaEnabled(const QString &fingerprint, bool enabled,
                                      const QString &source = QStringLiteral("operator"));
    //! Aplica uma alteração parcial no perfil de transmissão da saída.
    CommandResult requestBroadcastProfile(const QString &fingerprint, const QVariantMap &changes,
                                          const QString &source = QStringLiteral("operator"));

private:
    CommandResult dispatch(const QString &type, const QVariantMap &payload,
                           const QString &source);
    bool applyEnabled(const QString &fingerprint, bool enabled, const QString &correlationId);
    bool applyRole(const QString &fingerprint, const QString &role, const QString &correlationId);
    bool applyMediaEnabled(const QString &fingerprint, bool enabled,
                           const QString &correlationId);
    bool applyBroadcastProfile(const QString &fingerprint, const QVariantMap &profile,
                               const QString &correlationId);
    bool publishState(const QString &fingerprint, const QString &action,
                      const QString &correlationId);
    [[nodiscard]] CommandResult invalidPayload(const QString &message) const;
    CommandBus &m_commandBus;
    EventBus &m_eventBus;
    Actions m_actions;
    UndoManager *m_undoManager = nullptr;
};

} // namespace churchpresenter
