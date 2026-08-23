#pragma once

#include <QJsonObject>
#include <QString>

#include <functional>

namespace churchpresenter {

struct ObsResponse {
    bool completed = false;
    bool success = false;
    QString errorCode;
    QString message;
    QJsonObject data;
};

//! Porta do cliente OBS WebSocket v5. A implementação cuida do handshake, da
//! autenticação e da correlação de `requestId`.
class IObsClient {
public:
    using Completion = std::function<void(const ObsResponse &)>;

    virtual ~IObsClient() = default;

    //! Conecta e identifica. A senha nunca vai na URL nem em log.
    virtual void connectAndIdentify(const QString &host, quint16 port, const QString &password,
                                    int timeoutMs, Completion completion) = 0;
    virtual void request(const QString &requestType, const QJsonObject &requestData,
                         int timeoutMs, Completion completion) = 0;
    virtual void disconnect() = 0;
    [[nodiscard]] virtual QString state() const = 0;
    virtual void cancelAll() = 0;
};

} // namespace churchpresenter
