#include "core/CommandCatalog.h"

#include <algorithm>

namespace churchpresenter {

QList<CommandDescriptor> CommandCatalog::descriptors()
{
    return {
        {QStringLiteral("presentation.slide.show"), true, false},
        {QStringLiteral("presentation.slide.next"), true, false},
        {QStringLiteral("presentation.slide.previous"), true, false},
        {QStringLiteral("presentation.slide.first"), true, false},
        {QStringLiteral("presentation.slide.last"), true, false},
        {QStringLiteral("presentation.stop"), true, false},
        {QStringLiteral("presentation.blackout.set"), true, true},
        {QStringLiteral("media.play"), true, false},
        {QStringLiteral("media.pause.toggle"), true, false},
        {QStringLiteral("media.stop"), true, false},
        {QStringLiteral("media.seek"), true, false},
        {QStringLiteral("media.previous"), true, false},
        {QStringLiteral("media.next"), true, false},
        {QStringLiteral("media.repeat.set"), true, true},
        {QStringLiteral("media.playlist.move"), false, true},
        {QStringLiteral("media.playlist.remove"), false, true},
        {QStringLiteral("media.playlist.clear"), false, true},
        {QStringLiteral("bible.reference.present"), true, false},
        {QStringLiteral("bible.search"), true, false},
        {QStringLiteral("event.select"), true, false},
        {QStringLiteral("event.item.execute"), true, false},
        {QStringLiteral("stage.message.set"), true, true},
        {QStringLiteral("overlay.audience-message.set"), true, true},
        {QStringLiteral("overlay.alert.set"), true, true},
        {QStringLiteral("overlay.lower-third.set"), true, true},
        {QStringLiteral("timer.countdown.start"), true, false},
        {QStringLiteral("timer.countdown.stop"), true, false},
        {QStringLiteral("timer.stopwatch.start"), true, false},
        {QStringLiteral("timer.stopwatch.pause"), true, false},
        {QStringLiteral("timer.stopwatch.reset"), true, false},
        {QStringLiteral("output.enabled.set"), false, true},
        {QStringLiteral("output.role.set"), false, true},
        {QStringLiteral("output.media-enabled.set"), false, true},
        {QStringLiteral("output.broadcast-profile.set"), false, true},
        {QStringLiteral("settings.theme.apply"), false, true},
        {QStringLiteral("system.undo"), false, false},
        {QStringLiteral("system.redo"), false, false},
    };
}

bool CommandCatalog::contains(const QString &type)
{
    const auto commands = descriptors();
    return std::any_of(commands.cbegin(), commands.cend(), [&type](const auto &descriptor) {
        return descriptor.type == type;
    });
}

bool CommandCatalog::isRemoteAllowed(const QString &type)
{
    const auto commands = descriptors();
    const auto found = std::find_if(commands.cbegin(), commands.cend(),
                                    [&type](const auto &descriptor) {
        return descriptor.type == type;
    });
    return found != commands.cend() && found->remoteAllowed;
}

} // namespace churchpresenter
