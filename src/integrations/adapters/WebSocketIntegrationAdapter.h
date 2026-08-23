#pragma once

#include "integrations/ports/IIntegrationAdapter.h"
#include "integrations/ports/ITransports.h"

namespace churchpresenter {

//! Adapter de cliente WebSocket. Conecta, envia texto ou JSON e desconecta,
//! sempre com limite de mensagem e estado observável.
class WebSocketIntegrationAdapter final : public IIntegrationAdapter {
public:
    static constexpr int MaximumMessageBytes = 64 * 1024;

    explicit WebSocketIntegrationAdapter(IWebSocketTransport &transport);

    [[nodiscard]] IntegrationValidation validate(
        const IntegrationDefinition &definition) const override;
    void test(const IntegrationDefinition &definition, Completion completion) override;
    void execute(const IntegrationDefinition &definition, const IntegrationRequest &request,
                 Completion completion) override;
    void cancelAll() override;
    [[nodiscard]] bool isRetriable(const IntegrationDefinition &definition,
                                   const QString &operation) const override;

    [[nodiscard]] static QStringList supportedOperations();
    //! Estado atual da conexão configurada na definição.
    [[nodiscard]] QString state(const IntegrationDefinition &definition) const;

private:
    [[nodiscard]] static QString urlOf(const IntegrationDefinition &definition);
    [[nodiscard]] static QString messageOf(const IntegrationDefinition &definition,
                                           const IntegrationRequest &request);

    IWebSocketTransport &m_transport;
};

} // namespace churchpresenter
