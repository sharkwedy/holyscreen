#include "automation/TriggerTranslator.h"

namespace churchpresenter {
namespace {

QString actionOf(const DomainEvent &event)
{
    return event.payload.value(QStringLiteral("action")).toString();
}

} // namespace

std::optional<TriggerTranslator::Match> TriggerTranslator::translate(const DomainEvent &event)
{
    const auto action = actionOf(event);
    const auto payload = event.payload;
    const auto match = [&](const char *trigger) {
        return Match{.triggerType = QString::fromLatin1(trigger),
                     .payload = payload,
                     .correlationId = event.correlationId};
    };

    if (event.type == QStringLiteral("presentation.state.changed")) {
        if (action == QStringLiteral("stop")) return match(AutomationTrigger::PresentationStopped);
        if (action == QStringLiteral("slide.show")) {
            // Mostrar o primeiro slide de uma apresentação é o começo dela.
            const auto index = payload.value(QStringLiteral("slideIndex"), -1).toInt();
            if (index == 0) return match(AutomationTrigger::PresentationStarted);
            return match(AutomationTrigger::SlideChanged);
        }
        if (action.startsWith(QStringLiteral("slide."))) {
            return match(AutomationTrigger::SlideChanged);
        }
        return std::nullopt;
    }

    if (event.type == QStringLiteral("media.state.changed")) {
        if (action == QStringLiteral("media.play")) return match(AutomationTrigger::MediaStarted);
        if (action == QStringLiteral("media.stop")) return match(AutomationTrigger::MediaFinished);
        if (action == QStringLiteral("media.pause.toggle")) {
            const auto state = payload.value(QStringLiteral("state")).toString();
            return state == QStringLiteral("playing") ? match(AutomationTrigger::MediaStarted)
                                                      : match(AutomationTrigger::MediaPaused);
        }
        return std::nullopt;
    }

    if (event.type == QStringLiteral("event.state.changed")) {
        if (action == QStringLiteral("select")) return match(AutomationTrigger::EventSelected);
        if (action == QStringLiteral("item.execute")) {
            return match(AutomationTrigger::EventItemExecuted);
        }
        return std::nullopt;
    }

    if (event.type == QStringLiteral("automation.timer.started")) {
        return match(AutomationTrigger::TimerStarted);
    }
    if (event.type == QStringLiteral("automation.timer.finished")) {
        return match(AutomationTrigger::TimerFinished);
    }
    if (event.type == QStringLiteral("automation.song.started")) {
        return match(AutomationTrigger::SongStarted);
    }
    if (event.type == QStringLiteral("automation.remote.command")) {
        return match(AutomationTrigger::RemoteCommandAccepted);
    }
    return std::nullopt;
}

} // namespace churchpresenter
