#pragma once

#include <QString>

#include <QtGlobal>

namespace churchpresenter {

class AppLogger final {
public:
    static void install(const QString &directory);
    static QString logPath();
    static void setDebugMessagesEnabled(bool enabled);
    static bool debugMessagesEnabled();

    //! Contagens acumuladas desde a instalação do manipulador. O modo de
    //! endurance usa esses números para transformar um log crítico em
    //! bloqueador sem precisar reler o arquivo de log.
    static quint64 warningCount();
    static quint64 criticalCount();
    static quint64 fatalCount();
    static void resetCounters();
};

} // namespace churchpresenter
