#pragma once

#include "integrations/ports/IIntegrationAdapter.h"
#include "integrations/ports/ITransports.h"

namespace churchpresenter {

//! Adapter OSC sobre UDP. Envia mensagens para um host IPv4 ou IPv6 e nunca
//! abre porta de escuta.
class OscIntegrationAdapter final : public IIntegrationAdapter {
public:
    explicit OscIntegrationAdapter(IOscTransport &transport);

    [[nodiscard]] IntegrationValidation validate(
        const IntegrationDefinition &definition) const override;
    void test(const IntegrationDefinition &definition, Completion completion) override;
    void execute(const IntegrationDefinition &definition, const IntegrationRequest &request,
                 Completion completion) override;
    void cancelAll() override;

private:
    IOscTransport &m_transport;
};

} // namespace churchpresenter
