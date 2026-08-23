#pragma once

#include "integrations/ports/ITransports.h"

#include <QHash>

#include <memory>

class RtMidiOut;

namespace churchpresenter {

//! Transporte MIDI sobre RtMidi. Reconsulta as portas a cada chamada para
//! suportar hot-plug e nunca lança exceção para a camada de aplicação.
class RtMidiTransport final : public IMidiTransport {
public:
    RtMidiTransport();
    ~RtMidiTransport() override;

    RtMidiTransport(const RtMidiTransport &) = delete;
    RtMidiTransport &operator=(const RtMidiTransport &) = delete;

    [[nodiscard]] QStringList outputPorts() const override;
    bool openPort(const QString &portName) override;
    bool send(const QString &portName, const MidiMessage &message) override;
    void closeAll() override;
    [[nodiscard]] QString lastError() const override;

private:
    [[nodiscard]] int indexOfPort(const QString &portName) const;

    std::unique_ptr<RtMidiOut> m_client;
    QHash<QString, std::shared_ptr<RtMidiOut>> m_openPorts;
    mutable QString m_lastError;
};

} // namespace churchpresenter
