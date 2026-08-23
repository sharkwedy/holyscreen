#pragma once

#include "integrations/IntegrationTypes.h"

#include <QVector>

namespace churchpresenter {

//! Persistência das definições e do histórico sanitizado de chamadas.
class IIntegrationRepository {
public:
    virtual ~IIntegrationRepository() = default;

    [[nodiscard]] virtual QVector<IntegrationDefinition> definitions() const = 0;
    virtual bool save(const IntegrationDefinition &definition) = 0;
    virtual bool remove(const QString &integrationId) = 0;

    virtual bool recordCall(const IntegrationCall &call) = 0;
    [[nodiscard]] virtual QVector<IntegrationCall> history(const QString &integrationId,
                                                           int limit) const = 0;
    //! Aplica a retenção configurada, devolvendo quantos registros saíram.
    virtual int pruneHistory(int maximumEntriesPerIntegration) = 0;
};

} // namespace churchpresenter
