#pragma once

#include "integrations/ports/IObsClient.h"

#include <QHash>
#include <QObject>

QT_BEGIN_NAMESPACE
class QWebSocket;
class QTimer;
QT_END_NAMESPACE

namespace churchpresenter {

//! Cliente OBS WebSocket v5: Hello, Identify com desafio-resposta, Identified
//! e correlação de `requestId`. A senha nunca entra na URL nem em log.
class ObsWebSocketClient final : public QObject, public IObsClient {
    Q_OBJECT

public:
    static constexpr int SupportedRpcVersion = 1;

    explicit ObsWebSocketClient(QObject *parent = nullptr);
    ~ObsWebSocketClient() override;

    void connectAndIdentify(const QString &host, quint16 port, const QString &password,
                            int timeoutMs, Completion completion) override;
    void request(const QString &requestType, const QJsonObject &requestData, int timeoutMs,
                 Completion completion) override;
    void disconnect() override;
    [[nodiscard]] QString state() const override;
    void cancelAll() override;

private:
    void handleMessage(const QString &message);
    void failIdentification(const QString &errorCode, const QString &message);
    void send(const QJsonObject &payload);

    QWebSocket *m_socket = nullptr;
    QTimer *m_identifyTimeout = nullptr;
    QString m_state = QStringLiteral("disconnected");
    QString m_password;
    QList<Completion> m_identifyCompletions;
    QHash<QString, Completion> m_pendingRequests;
    QHash<QString, QTimer *> m_requestTimeouts;
    quint64 m_requestCounter = 0;
};

} // namespace churchpresenter
