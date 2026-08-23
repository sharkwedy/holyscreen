#include "integrations/transports/ObsWebSocketClient.h"

#include "integrations/adapters/ObsAuthentication.h"

#include <QJsonDocument>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>

namespace churchpresenter {
namespace {

constexpr int OpHello = 0;
constexpr int OpIdentify = 1;
constexpr int OpIdentified = 2;
constexpr int OpRequest = 6;
constexpr int OpRequestResponse = 7;

} // namespace

ObsWebSocketClient::ObsWebSocketClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_identifyTimeout(new QTimer(this))
{
    m_identifyTimeout->setSingleShot(true);

    connect(m_socket, &QWebSocket::textMessageReceived, this,
            &ObsWebSocketClient::handleMessage);
    connect(m_socket, &QWebSocket::disconnected, this, [this] {
        m_state = QStringLiteral("disconnected");
        failIdentification(QStringLiteral("connection_failed"),
                           QStringLiteral("A conexão com o OBS foi encerrada."));
    });
    connect(m_socket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        m_state = QStringLiteral("error");
        failIdentification(QStringLiteral("connection_failed"), m_socket->errorString());
    });
    connect(m_identifyTimeout, &QTimer::timeout, this, [this] {
        if (m_state == QStringLiteral("identified")) return;
        m_socket->abort();
        failIdentification(QStringLiteral("timeout"),
                           QStringLiteral("O OBS não respondeu ao handshake."));
    });
}

ObsWebSocketClient::~ObsWebSocketClient()
{
    cancelAll();
}

QString ObsWebSocketClient::state() const
{
    return m_state;
}

void ObsWebSocketClient::send(const QJsonObject &payload)
{
    m_socket->sendTextMessage(QString::fromUtf8(
        QJsonDocument(payload).toJson(QJsonDocument::Compact)));
    m_socket->flush();
}

void ObsWebSocketClient::failIdentification(const QString &errorCode, const QString &message)
{
    const auto pending = m_identifyCompletions;
    m_identifyCompletions.clear();
    for (const auto &completion : pending) {
        if (completion) {
            completion(ObsResponse{.completed = true,
                                   .success = false,
                                   .errorCode = errorCode,
                                   .message = message});
        }
    }

    const auto requests = m_pendingRequests;
    m_pendingRequests.clear();
    for (const auto &completion : requests) {
        if (completion) {
            completion(ObsResponse{.completed = true,
                                   .success = false,
                                   .errorCode = errorCode,
                                   .message = message});
        }
    }
    for (auto *timer : std::as_const(m_requestTimeouts)) {
        timer->stop();
        timer->deleteLater();
    }
    m_requestTimeouts.clear();
}

void ObsWebSocketClient::connectAndIdentify(const QString &host, quint16 port,
                                            const QString &password, int timeoutMs,
                                            Completion completion)
{
    if (m_state == QStringLiteral("identified")) {
        if (completion) completion(ObsResponse{.completed = true, .success = true});
        return;
    }
    m_password = password;
    if (completion) m_identifyCompletions.append(completion);
    if (m_socket->state() == QAbstractSocket::UnconnectedState) {
        m_state = QStringLiteral("connecting");
        QUrl url;
        url.setScheme(QStringLiteral("ws"));
        url.setHost(host);
        url.setPort(port);
        // A senha vai apenas no Identify, nunca na URL.
        m_socket->open(url);
    }
    m_identifyTimeout->start(std::max(500, timeoutMs));
}

void ObsWebSocketClient::handleMessage(const QString &message)
{
    const auto document = QJsonDocument::fromJson(message.toUtf8());
    if (!document.isObject()) return;
    const auto envelope = document.object();
    const auto op = envelope.value(QStringLiteral("op")).toInt(-1);
    const auto data = envelope.value(QStringLiteral("d")).toObject();

    if (op == OpHello) {
        QJsonObject identify{{QStringLiteral("rpcVersion"), SupportedRpcVersion}};
        const auto authentication = data.value(QStringLiteral("authentication")).toObject();
        if (!authentication.isEmpty()) {
            identify.insert(QStringLiteral("authentication"),
                            ObsAuthentication::response(
                                m_password,
                                authentication.value(QStringLiteral("salt")).toString(),
                                authentication.value(QStringLiteral("challenge")).toString()));
        }
        send(QJsonObject{{QStringLiteral("op"), OpIdentify}, {QStringLiteral("d"), identify}});
        return;
    }

    if (op == OpIdentified) {
        m_state = QStringLiteral("identified");
        m_identifyTimeout->stop();
        const auto pending = m_identifyCompletions;
        m_identifyCompletions.clear();
        for (const auto &completion : pending) {
            if (completion) {
                completion(ObsResponse{.completed = true, .success = true, .data = data});
            }
        }
        return;
    }

    if (op == OpRequestResponse) {
        const auto requestId = data.value(QStringLiteral("requestId")).toString();
        const auto completion = m_pendingRequests.take(requestId);
        if (auto *timer = m_requestTimeouts.take(requestId)) {
            timer->stop();
            timer->deleteLater();
        }
        if (!completion) return;
        const auto status = data.value(QStringLiteral("requestStatus")).toObject();
        const bool success = status.value(QStringLiteral("result")).toBool();
        completion(ObsResponse{
            .completed = true,
            .success = success,
            .errorCode = success ? QString{}
                                 : QStringLiteral("obs_error_%1")
                                       .arg(status.value(QStringLiteral("code")).toInt()),
            .message = status.value(QStringLiteral("comment")).toString(),
            .data = data.value(QStringLiteral("responseData")).toObject(),
        });
    }
}

void ObsWebSocketClient::request(const QString &requestType, const QJsonObject &requestData,
                                 int timeoutMs, Completion completion)
{
    if (m_state != QStringLiteral("identified")) {
        if (completion) {
            completion(ObsResponse{.completed = true,
                                   .success = false,
                                   .errorCode = QStringLiteral("connection_failed"),
                                   .message = QStringLiteral("O OBS não está identificado.")});
        }
        return;
    }
    const auto requestId = QStringLiteral("holyscreen-%1").arg(++m_requestCounter);
    if (completion) m_pendingRequests.insert(requestId, completion);

    auto *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, [this, requestId] {
        const auto pending = m_pendingRequests.take(requestId);
        if (auto *expired = m_requestTimeouts.take(requestId)) expired->deleteLater();
        if (pending) {
            pending(ObsResponse{.completed = true,
                                .success = false,
                                .errorCode = QStringLiteral("timeout"),
                                .message = QStringLiteral("O OBS não respondeu ao pedido.")});
        }
    });
    m_requestTimeouts.insert(requestId, timer);
    timer->start(std::max(250, timeoutMs));

    send(QJsonObject{{QStringLiteral("op"), OpRequest},
                     {QStringLiteral("d"),
                      QJsonObject{{QStringLiteral("requestType"), requestType},
                                  {QStringLiteral("requestId"), requestId},
                                  {QStringLiteral("requestData"), requestData}}}});
}

void ObsWebSocketClient::disconnect()
{
    m_identifyTimeout->stop();
    m_socket->close();
    m_state = QStringLiteral("disconnected");
}

void ObsWebSocketClient::cancelAll()
{
    m_identifyTimeout->stop();
    failIdentification(QStringLiteral("cancelled"), QStringLiteral("Chamada cancelada."));
    m_socket->abort();
    m_state = QStringLiteral("disconnected");
}

} // namespace churchpresenter
