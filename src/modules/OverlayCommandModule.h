#pragma once

#include "core/CommandBus.h"
#include "core/EventBus.h"
#include "presentation/OverlayController.h"

#include <QObject>

namespace churchpresenter {

class OverlayCommandModule final : public QObject {
    Q_OBJECT

public:
    OverlayCommandModule(CommandBus &commandBus, EventBus &eventBus,
                         OverlayController &overlays, QObject *parent = nullptr);

    CommandResult requestAudienceMessage(const QString &message,
                                         const QString &source = QStringLiteral("operator"));
    CommandResult requestAlert(const QString &message,
                               const QString &source = QStringLiteral("operator"));
    CommandResult requestLowerThird(const QString &title, const QString &subtitle,
                                    const QString &source = QStringLiteral("operator"));
    CommandResult requestCountdownStart(int seconds,
                                        const QString &source = QStringLiteral("operator"));
    CommandResult requestCountdownStop(const QString &source = QStringLiteral("operator"));
    CommandResult requestStopwatchStart(const QString &source = QStringLiteral("operator"));
    CommandResult requestStopwatchPause(const QString &source = QStringLiteral("operator"));
    CommandResult requestStopwatchReset(const QString &source = QStringLiteral("operator"));

private:
    CommandResult dispatch(const QString &type, const QVariantMap &payload,
                           const QString &source);
    CommandResult complete(const Command &command, const QString &message);
    [[nodiscard]] CommandResult invalidPayload(const QString &message) const;

    CommandBus &m_commandBus;
    EventBus &m_eventBus;
    OverlayController &m_overlays;
};

} // namespace churchpresenter
