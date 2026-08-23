#pragma once

#include "integrations/ports/IIntegrationRepository.h"

#include <QString>

namespace churchpresenter {

//! Adapter SQLite das definições e do histórico de integrações. O histórico
//! recebido já vem sanitizado pelo motor; este adapter nunca grava segredos.
class IntegrationRepository final : public IIntegrationRepository {
public:
    explicit IntegrationRepository(QString databasePath);
    ~IntegrationRepository() override;

    IntegrationRepository(const IntegrationRepository &) = delete;
    IntegrationRepository &operator=(const IntegrationRepository &) = delete;

    bool open();

    [[nodiscard]] QVector<IntegrationDefinition> definitions() const override;
    bool save(const IntegrationDefinition &definition) override;
    bool remove(const QString &integrationId) override;

    bool recordCall(const IntegrationCall &call) override;
    [[nodiscard]] QVector<IntegrationCall> history(const QString &integrationId,
                                                   int limit) const override;
    int pruneHistory(int maximumEntriesPerIntegration) override;

private:
    QString m_databasePath;
    QString m_connectionName;
};

} // namespace churchpresenter
