#pragma once

#include "integrations/ports/IIntegrationAdapter.h"
#include "integrations/ports/ITransports.h"

namespace churchpresenter {

//! Adapter MIDI. Envia Note On/Off, Control Change e Program Change para uma
//! porta de saída, com canal e valores validados.
class MidiIntegrationAdapter final : public IIntegrationAdapter {
public:
    static constexpr int MinimumChannel = 1;
    static constexpr int MaximumChannel = 16;
    static constexpr int MaximumValue = 127;

    explicit MidiIntegrationAdapter(IMidiTransport &transport);

    [[nodiscard]] IntegrationValidation validate(
        const IntegrationDefinition &definition) const override;
    void test(const IntegrationDefinition &definition, Completion completion) override;
    void execute(const IntegrationDefinition &definition, const IntegrationRequest &request,
                 Completion completion) override;
    void cancelAll() override;

    [[nodiscard]] static QStringList supportedOperations();
    //! Portas de saída disponíveis agora, para a interface e o diagnóstico.
    [[nodiscard]] QStringList availablePorts() const;

private:
    IMidiTransport &m_transport;
};

} // namespace churchpresenter
