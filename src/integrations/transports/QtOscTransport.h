#pragma once

#include "integrations/ports/ITransports.h"

#include <QObject>
#include <QUdpSocket>

namespace churchpresenter {

//! Envio OSC por UDP. O socket é apenas de saída: nenhuma porta de escuta é
//! aberta por padrão.
class QtOscTransport final : public QObject, public IOscTransport {
    Q_OBJECT

public:
    explicit QtOscTransport(QObject *parent = nullptr);

    bool send(const QString &host, quint16 port, const QByteArray &datagram) override;
    [[nodiscard]] QString lastError() const override;

private:
    QUdpSocket m_socket;
    QString m_lastError;
};

} // namespace churchpresenter
