#include "integrations/transports/RtMidiTransport.h"

#include <QDebug>

#include <RtMidi.h>

namespace churchpresenter {

RtMidiTransport::RtMidiTransport()
{
    try {
        m_client = std::make_unique<RtMidiOut>();
    } catch (const RtMidiError &error) {
        m_lastError = QString::fromStdString(error.getMessage());
        qWarning() << "MIDI unavailable:" << m_lastError;
    }
}

RtMidiTransport::~RtMidiTransport()
{
    closeAll();
}

QStringList RtMidiTransport::outputPorts() const
{
    if (!m_client) return {};
    QStringList ports;
    try {
        const auto count = m_client->getPortCount();
        ports.reserve(static_cast<qsizetype>(count));
        for (unsigned int index = 0; index < count; ++index) {
            ports.append(QString::fromStdString(m_client->getPortName(index)));
        }
    } catch (const RtMidiError &error) {
        m_lastError = QString::fromStdString(error.getMessage());
        return {};
    }
    return ports;
}

int RtMidiTransport::indexOfPort(const QString &portName) const
{
    const auto ports = outputPorts();
    return static_cast<int>(ports.indexOf(portName));
}

bool RtMidiTransport::openPort(const QString &portName)
{
    if (!m_client) {
        m_lastError = QStringLiteral("Nenhum backend MIDI disponível.");
        return false;
    }
    if (m_openPorts.contains(portName)) return true;

    const auto index = indexOfPort(portName);
    if (index < 0) {
        m_lastError = QStringLiteral("A porta %1 não está conectada.").arg(portName);
        return false;
    }
    try {
        auto port = std::make_shared<RtMidiOut>();
        port->openPort(static_cast<unsigned int>(index),
                       QStringLiteral("HolyScreen").toStdString());
        m_openPorts.insert(portName, std::move(port));
    } catch (const RtMidiError &error) {
        m_lastError = QString::fromStdString(error.getMessage());
        return false;
    }
    m_lastError.clear();
    return true;
}

bool RtMidiTransport::send(const QString &portName, const MidiMessage &message)
{
    if (!openPort(portName)) return false;
    const auto port = m_openPorts.value(portName);
    if (!port) {
        m_lastError = QStringLiteral("A porta %1 não está aberta.").arg(portName);
        return false;
    }

    // Canal 1..16 na configuração, 0..15 no protocolo.
    const auto status = static_cast<unsigned char>(message.status | ((message.channel - 1) & 0x0F));
    std::vector<unsigned char> bytes{status, static_cast<unsigned char>(message.data1)};
    const bool hasSecondData = (message.status & 0xF0) != 0xC0;
    if (hasSecondData) bytes.push_back(static_cast<unsigned char>(message.data2));

    try {
        port->sendMessage(&bytes);
    } catch (const RtMidiError &error) {
        m_lastError = QString::fromStdString(error.getMessage());
        // Uma porta removida durante o culto não pode derrubar o aplicativo.
        m_openPorts.remove(portName);
        return false;
    }
    m_lastError.clear();
    return true;
}

void RtMidiTransport::closeAll()
{
    for (const auto &port : std::as_const(m_openPorts)) {
        try {
            if (port && port->isPortOpen()) port->closePort();
        } catch (const RtMidiError &error) {
            qWarning() << "Could not close the MIDI port:"
                       << QString::fromStdString(error.getMessage());
        }
    }
    m_openPorts.clear();
}

QString RtMidiTransport::lastError() const
{
    return m_lastError;
}

} // namespace churchpresenter
