#pragma once

#include "integrations/ports/IIntegrationAdapter.h"
#include "integrations/ports/IObsClient.h"
#include "integrations/ports/ISecretStore.h"

namespace churchpresenter {

//! Adapter do OBS WebSocket v5. Troca cena, controla gravação e transmissão e
//! aciona inputs, sempre pelo cliente identificado.
class ObsIntegrationAdapter final : public IIntegrationAdapter {
public:
    ObsIntegrationAdapter(IObsClient &client, const ISecretStore *secretStore = nullptr);

    [[nodiscard]] IntegrationValidation validate(
        const IntegrationDefinition &definition) const override;
    void test(const IntegrationDefinition &definition, Completion completion) override;
    void execute(const IntegrationDefinition &definition, const IntegrationRequest &request,
                 Completion completion) override;
    void cancelAll() override;
    [[nodiscard]] bool isRetriable(const IntegrationDefinition &definition,
                                   const QString &operation) const override;

    [[nodiscard]] static QStringList supportedOperations();

private:
    struct ObsCall {
        QString requestType;
        QJsonObject requestData;
        QString errorCode;
        QString message;
    };

    [[nodiscard]] static ObsCall callFor(const IntegrationRequest &request);
    [[nodiscard]] QString passwordOf(const IntegrationDefinition &definition) const;
    void withConnection(const IntegrationDefinition &definition, Completion onFailure,
                        std::function<void()> onReady);

    IObsClient &m_client;
    const ISecretStore *m_secretStore = nullptr;
};

} // namespace churchpresenter
