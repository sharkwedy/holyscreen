#pragma once

#include "integrations/ports/IIntegrationAdapter.h"
#include "integrations/ports/ISecretStore.h"
#include "integrations/ports/ITransports.h"

namespace churchpresenter {

//! Adapter HTTP de saída. Aceita apenas `http` e `https`, resolve segredos
//! pelo cofre do sistema e devolve status, duração e cabeçalhos permitidos.
class HttpIntegrationAdapter final : public IIntegrationAdapter {
public:
    static constexpr qint64 MaximumResponseBytes = 512 * 1024;
    static constexpr int MaximumHeaders = 32;
    static constexpr int MaximumBodyBytes = 256 * 1024;

    HttpIntegrationAdapter(IHttpTransport &transport, const ISecretStore *secretStore = nullptr);

    [[nodiscard]] IntegrationValidation validate(
        const IntegrationDefinition &definition) const override;
    void test(const IntegrationDefinition &definition, Completion completion) override;
    void execute(const IntegrationDefinition &definition, const IntegrationRequest &request,
                 Completion completion) override;
    void cancelAll() override;
    [[nodiscard]] bool isRetriable(const IntegrationDefinition &definition,
                                   const QString &operation) const override;

    //! Métodos aceitos na configuração.
    [[nodiscard]] static QStringList supportedMethods();
    //! Cabeçalhos de resposta que podem aparecer no resultado e no histórico.
    [[nodiscard]] static QStringList allowedResponseHeaders();
    //! Substitui `{{chave}}` pelos valores do payload do pedido.
    [[nodiscard]] static QString applyTemplate(const QString &source, const QVariantMap &payload);

private:
    [[nodiscard]] HttpRequest buildRequest(const IntegrationDefinition &definition,
                                           const IntegrationRequest &request) const;
    [[nodiscard]] QString resolveSecret(const QString &reference) const;

    IHttpTransport &m_transport;
    const ISecretStore *m_secretStore = nullptr;
};

} // namespace churchpresenter
