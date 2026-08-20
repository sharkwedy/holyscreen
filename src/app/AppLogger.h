#pragma once

#include <QString>

namespace churchpresenter {

class AppLogger final {
public:
    static void install(const QString &directory);
    static QString logPath();
    static void setDebugMessagesEnabled(bool enabled);
    static bool debugMessagesEnabled();
};

} // namespace churchpresenter
