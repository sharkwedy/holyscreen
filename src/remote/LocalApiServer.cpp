#include "remote/LocalApiServer.h"

#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QHttpServerWebSocketUpgradeResponse>
#include <QJsonDocument>
#include <QNetworkInterface>
#include <QStringList>
#include <QWebSocket>

namespace churchpresenter {

LocalApiServer::LocalApiServer(QObject *parent) : QObject(parent)
{
    configureRoutes();
}

LocalApiServer::~LocalApiServer() { stop(); }

void LocalApiServer::configureRoutes()
{
    m_http.route(QStringLiteral("/"), QHttpServerRequest::Method::Get, [] {
        return QHttpServerResponse("text/html; charset=utf-8", remotePage());
    });
    m_http.route(QStringLiteral("/api/health"), QHttpServerRequest::Method::Get, [this] {
        return QJsonObject{{QStringLiteral("ok"), true}, {QStringLiteral("service"), QStringLiteral("HolyScreen")}};
    });
    m_http.route(QStringLiteral("/api/state"), QHttpServerRequest::Method::Get, [this] { return state(); });
    m_http.route(QStringLiteral("/api/command"), QHttpServerRequest::Method::Post,
                 [this](const QHttpServerRequest &request) {
        return processCommandPayload(request.body())
            ? QHttpServerResponse(QJsonObject{{QStringLiteral("accepted"), true}})
            : QHttpServerResponse(QJsonObject{{QStringLiteral("accepted"), false}},
                                  QHttpServerResponse::StatusCode::BadRequest);
    });
    m_http.addWebSocketUpgradeVerifier(this, [](const QHttpServerRequest &request) {
        return request.url().path() == QStringLiteral("/ws")
            ? QHttpServerWebSocketUpgradeResponse::accept()
            : QHttpServerWebSocketUpgradeResponse::passToNext();
    });
    connect(&m_http, &QHttpServer::newWebSocketConnection, this, [this] {
        auto owned = m_http.nextPendingWebSocketConnection();
        if (!owned) return;
        auto *socket = owned.release();
        socket->setParent(this);
        m_clients.append(socket);
        connect(socket, &QWebSocket::textMessageReceived, this, [this, socket](const QString &message) {
            const auto accepted = processCommandPayload(message.toUtf8());
            socket->sendTextMessage(QString::fromUtf8(QJsonDocument(
                QJsonObject{{QStringLiteral("type"), QStringLiteral("ack")},
                            {QStringLiteral("accepted"), accepted}}).toJson(QJsonDocument::Compact)));
        });
        connect(socket, &QWebSocket::disconnected, this, [this, socket] {
            m_clients.removeAll(socket); socket->deleteLater(); emit statusChanged();
        });
        emit statusChanged();
        broadcastState();
    });
}

bool LocalApiServer::start(quint16 requestedPort)
{
    if (running()) return true;
    m_lastError.clear();
    if (!m_tcp.listen(QHostAddress::AnyIPv4, requestedPort)) {
        m_lastError = QStringLiteral("Não foi possível abrir a porta %1 em IPv4: %2")
                          .arg(requestedPort).arg(m_tcp.errorString());
        qWarning().noquote() << "remote_bind_failed" << m_lastError;
        emit statusChanged();
        return false;
    }
    if (!m_http.bind(&m_tcp)) {
        QStringList registeredPorts;
        for (const auto registeredPort : m_http.serverPorts())
            registeredPorts.append(QString::number(registeredPort));
        m_lastError = QStringLiteral("O servidor HTTP recusou a porta %1; TCP ativo=%2; portas HTTP=%3")
                          .arg(m_tcp.serverPort())
                          .arg(m_tcp.isListening() ? QStringLiteral("sim") : QStringLiteral("não"),
                               registeredPorts.join(QLatin1Char(',')));
        qWarning().noquote() << "remote_http_bind_failed" << m_lastError;
        m_tcp.close();
        emit statusChanged();
        return false;
    }
    emit statusChanged();
    return true;
}

void LocalApiServer::stop()
{
    for (auto *client : std::as_const(m_clients)) client->close();
    m_clients.clear();
    m_tcp.close();
    emit statusChanged();
}

bool LocalApiServer::running() const { return m_tcp.isListening(); }
quint16 LocalApiServer::port() const { return m_tcp.serverPort(); }

QString LocalApiServer::localAddress() const
{
    for (const auto &address : QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback())
            return address.toString();
    }
    return QStringLiteral("127.0.0.1");
}

QString LocalApiServer::remoteUrl() const
{
    return running() ? QStringLiteral("http://%1:%2").arg(localAddress()).arg(port()) : QString{};
}

int LocalApiServer::clientCount() const { return m_clients.size(); }
QString LocalApiServer::lastError() const { return m_lastError; }
void LocalApiServer::setStateProvider(std::function<QJsonObject()> provider) { m_stateProvider = std::move(provider); }

bool LocalApiServer::processCommandPayload(const QByteArray &payload)
{
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) return false;
    const auto object = document.object();
    const auto command = object.value(QStringLiteral("command")).toString().trimmed();
    if (command.isEmpty()) return false;
    emit commandReceived(command, object.value(QStringLiteral("arguments")).toObject());
    return true;
}

QJsonObject LocalApiServer::state() const { return m_stateProvider ? m_stateProvider() : QJsonObject{}; }

void LocalApiServer::broadcastState()
{
    const auto payload = QString::fromUtf8(QJsonDocument(
        QJsonObject{{QStringLiteral("type"), QStringLiteral("state")},
                    {QStringLiteral("data"), state()}}).toJson(QJsonDocument::Compact));
    for (auto *client : std::as_const(m_clients)) client->sendTextMessage(payload);
}

QByteArray LocalApiServer::remotePage()
{
    return QByteArrayLiteral(R"HTML(<!doctype html><html lang="pt-BR"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>HolyScreen Remoto</title><style>body{margin:0;background:#08111f;color:#edf6ff;font:16px system-ui}main{max-width:620px;margin:auto;padding:22px}h1{letter-spacing:2px}.grid{display:grid;grid-template-columns:repeat(2,1fr);gap:12px}button,input{font:inherit;border:0;border-radius:12px;padding:17px}button{background:#18345a;color:white;font-weight:700}button.primary{background:#2563eb}button.danger{background:#b91c1c}input{width:100%;box-sizing:border-box;margin:10px 0;background:#152137;color:white}#status{color:#70e1a7}</style>
<main><h1>HOLYSCREEN</h1><p id="status">Conectando…</p><div class="grid"><button onclick="send('presentation.previous')">◀ ANTERIOR</button><button class="primary" onclick="send('presentation.next')">PRÓXIMO ▶</button><button onclick="send('media.playPause')">TOCAR / PAUSAR</button><button onclick="send('media.stop')">PARAR MÍDIA</button><button class="danger" onclick="send('blackout.toggle')">BLACKOUT</button><button onclick="send('presentation.first')">PRIMEIRO SLIDE</button></div><input id="msg" placeholder="Mensagem ao público"><button onclick="send('overlay.message',{text:msg.value})">EXIBIR MENSAGEM</button><input id="stage" placeholder="Mensagem ao palco"><button onclick="send('stage.message',{text:stage.value})">ENVIAR AO PALCO</button></main>
<script>let ws;function connect(){ws=new WebSocket(`ws://${location.host}/ws`);ws.onopen=()=>status.textContent='Conectado';ws.onclose=()=>{status.textContent='Reconectando…';setTimeout(connect,1200)}}function send(command,arguments={}){const data=JSON.stringify({command,arguments});if(ws&&ws.readyState===1)ws.send(data);else fetch('/api/command',{method:'POST',headers:{'Content-Type':'application/json'},body:data})}connect()</script></html>)HTML");
}

} // namespace churchpresenter
