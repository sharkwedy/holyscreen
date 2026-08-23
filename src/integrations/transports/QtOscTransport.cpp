#include "integrations/transports/QtOscTransport.h"

#include "integrations/adapters/OscMessage.h"

#include <QHostAddress>

namespace churchpresenter {

QtOscTransport::QtOscTransport(QObject *parent)
    : QObject(parent)
{
}

bool QtOscTransport::send(const QString &host, quint16 port, const QByteArray &datagram)
{
    QHostAddress address;
    if (!address.setAddress(host)) {
        m_lastError = QStringLiteral("Endereço inválido: %1.").arg(host);
        return false;
    }
    if (datagram.size() > OscMessage::MaximumDatagramBytes) {
        m_lastError = QStringLiteral("O datagrama excede %1 bytes.")
                          .arg(OscMessage::MaximumDatagramBytes);
        return false;
    }
    const auto written = m_socket.writeDatagram(datagram, address, port);
    if (written != datagram.size()) {
        m_lastError = m_socket.errorString();
        return false;
    }
    m_lastError.clear();
    return true;
}

QString QtOscTransport::lastError() const
{
    return m_lastError;
}

} // namespace churchpresenter
