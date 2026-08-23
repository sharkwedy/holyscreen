#pragma once

#include "automation/AuthorizedExecutables.h"
#include "automation/AutomationTypes.h"

#include <QString>

namespace churchpresenter {

//! Persistência das automações, do histórico de execuções e da allowlist de
//! executáveis.
class AutomationRepository final {
public:
    static constexpr int DefaultRunRetention = 300;

    explicit AutomationRepository(QString databasePath);
    ~AutomationRepository();

    AutomationRepository(const AutomationRepository &) = delete;
    AutomationRepository &operator=(const AutomationRepository &) = delete;

    bool open();

    [[nodiscard]] QList<Automation> automations() const;
    bool save(const Automation &automation);
    bool remove(const QString &automationId);
    //! Guarda apenas o estado que muda em execução, sem reescrever a definição.
    bool updateRuntimeState(const QString &automationId, bool enabled, int consecutiveFailures);

    bool recordRun(const AutomationRun &run);
    [[nodiscard]] QList<AutomationRun> runs(const QString &automationId, int limit) const;
    int pruneRuns(int maximumEntriesPerAutomation);

    [[nodiscard]] QList<AuthorizedExecutables::Entry> authorizedExecutables() const;
    bool authorizeExecutable(const AuthorizedExecutables::Entry &entry);
    bool revokeExecutable(const QString &canonicalPath);

private:
    QString m_databasePath;
    QString m_connectionName;
};

} // namespace churchpresenter
