#include "remote/LocalApiServer.h"

#include "core/CommandCatalog.h"

#include <QDateTime>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QHttpServerWebSocketUpgradeResponse>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkInterface>
#include <QStringList>
#include <QTimer>
#include <QUuid>
#include <QWebSocket>

namespace {

using Status = QHttpServerResponse::StatusCode;

QHttpServerResponse jsonError(Status status, const QString &code, const QString &message)
{
    return QHttpServerResponse(QJsonObject{
        {QStringLiteral("accepted"), false},
        {QStringLiteral("errorCode"), code},
        {QStringLiteral("message"), message},
    }, status);
}

QJsonObject commandResultJson(const churchpresenter::CommandResult &result)
{
    return {
        {QStringLiteral("accepted"), result.accepted},
        {QStringLiteral("errorCode"), result.errorCode},
        {QStringLiteral("message"), result.message},
        {QStringLiteral("stateRevision"), static_cast<qint64>(result.stateRevision)},
    };
}

QJsonObject parseObject(const QByteArray &payload, bool *ok)
{
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(payload, &parseError);
    *ok = parseError.error == QJsonParseError::NoError && document.isObject();
    return *ok ? document.object() : QJsonObject{};
}

} // namespace

namespace churchpresenter {

LocalApiServer::LocalApiServer(QObject *parent)
    : QObject(parent)
{
    configureRoutes();
}

LocalApiServer::~LocalApiServer() { stop(); }

void LocalApiServer::configureRoutes()
{
    m_http.route(QStringLiteral("/"), QHttpServerRequest::Method::Get, [] {
        return QHttpServerResponse("text/html; charset=utf-8", remotePage());
    });
    m_http.route(QStringLiteral("/manifest.webmanifest"), QHttpServerRequest::Method::Get, [] {
        return QHttpServerResponse("application/manifest+json", manifest());
    });
    m_http.route(QStringLiteral("/sw.js"), QHttpServerRequest::Method::Get, [] {
        return QHttpServerResponse("application/javascript; charset=utf-8", serviceWorker());
    });
    m_http.route(QStringLiteral("/api/v1/health"), QHttpServerRequest::Method::Get, [this] {
        return QHttpServerResponse(QJsonObject{
            {QStringLiteral("ok"), true},
            {QStringLiteral("service"), QStringLiteral("HolyScreen")},
            {QStringLiteral("apiVersion"), QStringLiteral("v1")},
            {QStringLiteral("passwordConfigured"), hasPassword()},
        });
    });
    m_http.route(QStringLiteral("/api/v1/openapi.json"), QHttpServerRequest::Method::Get, [] {
        return QHttpServerResponse(openApiDocument());
    });
    m_http.route(QStringLiteral("/api/v1/session"), QHttpServerRequest::Method::Post,
                 [this](const QHttpServerRequest &request) {
        if (!sameOrigin(request)) return jsonError(Status::Forbidden, QStringLiteral("invalid_origin"), QStringLiteral("Origem não permitida."));
        if (request.body().size() > MaximumPayloadBytes) return jsonError(Status::PayloadTooLarge, QStringLiteral("payload_too_large"), QStringLiteral("Payload acima de 64 KiB."));
        bool parsed = false;
        const auto object = parseObject(request.body(), &parsed);
        if (!parsed) return jsonError(Status::BadRequest, QStringLiteral("invalid_json"), QStringLiteral("JSON inválido."));
        const auto result = m_auth.login(object.value(QStringLiteral("password")).toString(),
                                         request.remoteAddress().toString());
        if (!result.accepted) {
            const auto status = result.errorCode == QStringLiteral("login_blocked")
                ? Status::TooManyRequests : Status::Unauthorized;
            return jsonError(status, result.errorCode, QStringLiteral("Não foi possível autenticar."));
        }
        emit statusChanged();
        return QHttpServerResponse(QJsonObject{
            {QStringLiteral("accepted"), true},
            {QStringLiteral("token"), result.token},
            {QStringLiteral("expiresAt"), result.expiresAt.toString(Qt::ISODateWithMs)},
        }, Status::Created);
    });
    m_http.route(QStringLiteral("/api/v1/session"), QHttpServerRequest::Method::Delete,
                 [this](const QHttpServerRequest &request) {
        if (!sameOrigin(request)) return jsonError(Status::Forbidden, QStringLiteral("invalid_origin"), QStringLiteral("Origem não permitida."));
        const auto token = bearerToken(request);
        if (!m_auth.validateToken(token)) return jsonError(Status::Unauthorized, QStringLiteral("invalid_session"), QStringLiteral("Sessão inválida ou expirada."));
        m_auth.logout(token);
        emit statusChanged();
        return QHttpServerResponse(Status::NoContent);
    });
    m_http.route(QStringLiteral("/api/v1/state"), QHttpServerRequest::Method::Get,
                 [this](const QHttpServerRequest &request) {
        if (!sameOrigin(request)) return jsonError(Status::Forbidden, QStringLiteral("invalid_origin"), QStringLiteral("Origem não permitida."));
        if (!m_auth.validateToken(bearerToken(request))) return jsonError(Status::Unauthorized, QStringLiteral("invalid_session"), QStringLiteral("Sessão inválida ou expirada."));
        return QHttpServerResponse(state());
    });
    m_http.route(QStringLiteral("/api/v1/commands"), QHttpServerRequest::Method::Post,
                 [this](const QHttpServerRequest &request) {
        if (!sameOrigin(request)) return jsonError(Status::Forbidden, QStringLiteral("invalid_origin"), QStringLiteral("Origem não permitida."));
        if (request.body().size() > MaximumPayloadBytes) return jsonError(Status::PayloadTooLarge, QStringLiteral("payload_too_large"), QStringLiteral("Payload acima de 64 KiB."));
        const auto token = bearerToken(request);
        if (!m_auth.validateToken(token)) return jsonError(Status::Unauthorized, QStringLiteral("invalid_session"), QStringLiteral("Sessão inválida ou expirada."));
        if (!m_auth.authorizeCommand(token)) return jsonError(Status::TooManyRequests, QStringLiteral("rate_limited"), QStringLiteral("Limite de comandos excedido."));
        bool parsed = false;
        const auto object = parseObject(request.body(), &parsed);
        if (!parsed) return jsonError(Status::BadRequest, QStringLiteral("invalid_json"), QStringLiteral("JSON inválido."));
        const auto hash = m_auth.sessionHash(token);
        const auto result = processCommand(object, hash,
            QStringLiteral("remote:%1").arg(QString::fromLatin1(hash.left(6).toHex())));
        if (result.accepted) m_lastStateRevision = result.stateRevision;
        const auto status = result.accepted ? Status::Ok
            : result.errorCode == QStringLiteral("unknown_command") ? Status::Forbidden
            : Status::BadRequest;
        return QHttpServerResponse(commandResultJson(result), status);
    });

    m_http.addWebSocketUpgradeVerifier(this, [this](const QHttpServerRequest &request) {
        return request.url().path() == QStringLiteral("/api/v1/ws") && sameOrigin(request)
            ? QHttpServerWebSocketUpgradeResponse::accept()
            : QHttpServerWebSocketUpgradeResponse::passToNext();
    });
    connect(&m_http, &QHttpServer::newWebSocketConnection, this, [this] {
        auto owned = m_http.nextPendingWebSocketConnection();
        if (!owned) return;
        auto *socket = owned.release();
        socket->setParent(this);
        m_clients.insert(socket, {});
        QTimer::singleShot(5000, socket, [this, socket] {
            if (m_clients.contains(socket) && m_clients.value(socket).isEmpty())
                socket->close(QWebSocketProtocol::CloseCodePolicyViolated,
                              QStringLiteral("authentication_timeout"));
        });
        connect(socket, &QWebSocket::textMessageReceived, this,
                [this, socket](const QString &message) {
            if (message.toUtf8().size() > MaximumPayloadBytes) {
                socket->close(QWebSocketProtocol::CloseCodeTooMuchData,
                              QStringLiteral("payload_too_large"));
                return;
            }
            bool parsed = false;
            const auto object = parseObject(message.toUtf8(), &parsed);
            if (!parsed) {
                sendWebSocketJson(socket, {{QStringLiteral("type"), QStringLiteral("error")},
                                           {QStringLiteral("errorCode"), QStringLiteral("invalid_json")}});
                return;
            }
            auto sessionHash = m_clients.value(socket);
            if (sessionHash.isEmpty()) {
                if (object.value(QStringLiteral("type")).toString() != QStringLiteral("authenticate")) {
                    socket->close(QWebSocketProtocol::CloseCodePolicyViolated,
                                  QStringLiteral("authentication_required"));
                    return;
                }
                sessionHash = m_auth.sessionHash(object.value(QStringLiteral("token")).toString());
                if (sessionHash.isEmpty()) {
                    socket->close(QWebSocketProtocol::CloseCodePolicyViolated,
                                  QStringLiteral("invalid_session"));
                    return;
                }
                m_clients[socket] = sessionHash;
                sendWebSocketJson(socket, {{QStringLiteral("type"), QStringLiteral("authenticated")}});
                sendWebSocketJson(socket, {{QStringLiteral("type"), QStringLiteral("state")},
                                           {QStringLiteral("data"), state()}});
                emit statusChanged();
                return;
            }
            if (!m_auth.validateSession(sessionHash)) {
                socket->close(QWebSocketProtocol::CloseCodePolicyViolated,
                              QStringLiteral("invalid_session"));
                return;
            }
            if (!m_auth.authorizeSession(sessionHash)) {
                sendWebSocketJson(socket, {{QStringLiteral("type"), QStringLiteral("error")},
                                           {QStringLiteral("errorCode"), QStringLiteral("rate_limited")}});
                return;
            }
            const auto commandObject = object.value(QStringLiteral("command")).isObject()
                ? object.value(QStringLiteral("command")).toObject() : object;
            const auto result = processCommand(commandObject, sessionHash,
                QStringLiteral("remote:%1").arg(QString::fromLatin1(sessionHash.left(6).toHex())));
            if (result.accepted) m_lastStateRevision = result.stateRevision;
            auto response = commandResultJson(result);
            response.insert(QStringLiteral("type"), QStringLiteral("commandResult"));
            sendWebSocketJson(socket, response);
        });
        connect(socket, &QWebSocket::disconnected, this, [this, socket] {
            m_clients.remove(socket);
            socket->deleteLater();
            emit statusChanged();
        });
        emit statusChanged();
    });
}

bool LocalApiServer::start(quint16 requestedPort, const QHostAddress &address)
{
    if (running()) return true;
    m_lastError.clear();
    if (!m_tcp.listen(address, requestedPort)) {
        m_lastError = QStringLiteral("Não foi possível abrir a porta %1 em %2: %3")
                          .arg(requestedPort).arg(address.toString(), m_tcp.errorString());
        qWarning().noquote() << "remote_bind_failed" << m_lastError;
        emit statusChanged();
        return false;
    }
    if (!m_http.bind(&m_tcp)) {
        QStringList registeredPorts;
        for (const auto registeredPort : m_http.serverPorts()) registeredPorts.append(QString::number(registeredPort));
        m_lastError = QStringLiteral("O servidor HTTP recusou a porta %1; TCP ativo=%2; portas HTTP=%3")
                          .arg(m_tcp.serverPort())
                          .arg(m_tcp.isListening() ? QStringLiteral("sim") : QStringLiteral("não"), registeredPorts.join(QLatin1Char(',')));
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
    for (auto *client : m_clients.keys()) client->close();
    m_clients.clear();
    m_tcp.close();
    emit statusChanged();
}

bool LocalApiServer::running() const { return m_tcp.isListening(); }
quint16 LocalApiServer::port() const { return m_tcp.serverPort(); }
QString LocalApiServer::localAddress() const
{
    if (running() && m_tcp.serverAddress() != QHostAddress::Any
        && m_tcp.serverAddress() != QHostAddress::AnyIPv4
        && m_tcp.serverAddress() != QHostAddress::AnyIPv6)
        return m_tcp.serverAddress().toString();
    for (const auto &address : QNetworkInterface::allAddresses())
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback()) return address.toString();
    return QStringLiteral("127.0.0.1");
}
QString LocalApiServer::remoteUrl() const { return running() ? QStringLiteral("http://%1:%2").arg(localAddress()).arg(port()) : QString{}; }
int LocalApiServer::clientCount() const
{
    int authenticated = 0;
    for (const auto &hash : m_clients) if (!hash.isEmpty()) ++authenticated;
    return authenticated;
}
int LocalApiServer::sessionCount() const { return m_auth.sessionCount(); }
QString LocalApiServer::lastError() const { return m_lastError; }
void LocalApiServer::setStateProvider(std::function<QJsonObject()> provider) { m_stateProvider = std::move(provider); }
void LocalApiServer::setCommandDispatcher(std::function<CommandResult(const Command &)> dispatcher) { m_commandDispatcher = std::move(dispatcher); }
QVariantMap LocalApiServer::setPassword(const QString &password)
{
    const auto result = m_auth.setPassword(password);
    if (!result.isEmpty()) {
        for (auto *client : m_clients.keys())
            client->close(QWebSocketProtocol::CloseCodePolicyViolated,
                          QStringLiteral("sessions_revoked"));
    }
    emit statusChanged();
    return result;
}
bool LocalApiServer::loadCredentials(const QVariantMap &credentials) { const auto result=m_auth.loadCredentials(credentials);emit statusChanged();return result; }
bool LocalApiServer::hasPassword() const { return m_auth.hasCredentials(); }
void LocalApiServer::revokeAllSessions()
{
    m_auth.revokeAll();
    for (auto *client : m_clients.keys()) client->close(QWebSocketProtocol::CloseCodePolicyViolated, QStringLiteral("sessions_revoked"));
    emit statusChanged();
}

QJsonObject LocalApiServer::state() const
{
    auto result = m_stateProvider ? m_stateProvider() : QJsonObject{};
    if (!result.contains(QStringLiteral("stateRevision"))) result.insert(QStringLiteral("stateRevision"), static_cast<qint64>(m_lastStateRevision));
    return result;
}

bool LocalApiServer::sameOrigin(const QHttpServerRequest &request) const
{
    const auto rawOrigin = request.value(QByteArrayLiteral("Origin"));
    if (rawOrigin.isEmpty()) return true;
    const QUrl origin(QString::fromUtf8(rawOrigin));
    const auto host = QString::fromUtf8(request.value(QByteArrayLiteral("Host")));
    return origin.isValid() && !host.isEmpty()
        && origin.authority().compare(host, Qt::CaseInsensitive) == 0
        && origin.scheme() == QStringLiteral("http");
}

QString LocalApiServer::bearerToken(const QHttpServerRequest &request) const
{
    const auto authorization = request.value(QByteArrayLiteral("Authorization"));
    constexpr auto prefix = "Bearer ";
    return authorization.startsWith(prefix) ? QString::fromUtf8(authorization.mid(7)).trimmed() : QString{};
}

CommandResult LocalApiServer::processCommand(const QJsonObject &object,
                                             const QByteArray &sessionHash,
                                             const QString &source)
{
    const auto id = object.value(QStringLiteral("id")).toString().trimmed();
    const auto type = object.value(QStringLiteral("type")).toString().trimmed();
    if (id.isEmpty() || type.isEmpty() || !object.value(QStringLiteral("payload")).isObject())
        return {.accepted=false,.errorCode=QStringLiteral("invalid_payload"),.message=QStringLiteral("id, type e payload são obrigatórios."),.stateRevision=m_lastStateRevision};
    if (sessionHash.isEmpty()) return {.accepted=false,.errorCode=QStringLiteral("invalid_session"),.message=QStringLiteral("Sessão inválida."),.stateRevision=m_lastStateRevision};
    if (!CommandCatalog::isRemoteAllowed(type)) return {.accepted=false,.errorCode=QStringLiteral("unknown_command"),.message=QStringLiteral("Comando remoto não permitido."),.stateRevision=m_lastStateRevision};
    if (!m_commandDispatcher) return {.accepted=false,.errorCode=QStringLiteral("service_unavailable"),.message=QStringLiteral("Controlador indisponível."),.stateRevision=m_lastStateRevision};
    return m_commandDispatcher(Command{.id=id,.type=type,.payload=object.value(QStringLiteral("payload")).toObject().toVariantMap(),.source=source,.issuedAt=QDateTime::currentDateTimeUtc()});
}

void LocalApiServer::sendWebSocketJson(QWebSocket *socket, const QJsonObject &object)
{
    if (socket) socket->sendTextMessage(QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact)));
}

void LocalApiServer::broadcastState()
{
    const QJsonObject payload{{QStringLiteral("type"), QStringLiteral("state")}, {QStringLiteral("data"), state()}};
    for (auto iterator=m_clients.cbegin();iterator!=m_clients.cend();++iterator) if(!iterator.value().isEmpty())sendWebSocketJson(iterator.key(),payload);
}

void LocalApiServer::broadcastEvent(const DomainEvent &event, quint64 stateRevision)
{
    m_lastStateRevision = stateRevision;
    const QJsonObject payload{
        {QStringLiteral("type"), QStringLiteral("event")},
        {QStringLiteral("event"), QJsonObject{
            {QStringLiteral("type"), event.type},
            {QStringLiteral("payload"), QJsonObject::fromVariantMap(event.payload)},
            {QStringLiteral("occurredAt"), event.occurredAt.toString(Qt::ISODateWithMs)},
            {QStringLiteral("correlationId"), event.correlationId},
            {QStringLiteral("stateRevision"), static_cast<qint64>(stateRevision)},
        }},
    };
    for (auto iterator=m_clients.cbegin();iterator!=m_clients.cend();++iterator) if(!iterator.value().isEmpty())sendWebSocketJson(iterator.key(),payload);
}

QJsonObject LocalApiServer::openApiDocument()
{
    const auto schemaReference = [](const QString &name) {
        return QJsonObject{{QStringLiteral("$ref"),
                            QStringLiteral("#/components/schemas/%1").arg(name)}};
    };
    const QJsonArray bearerSecurity{
        QJsonObject{{QStringLiteral("bearerAuth"), QJsonArray{}}}};
    const auto jsonBody = [&schemaReference](const QString &schema) {
        return QJsonObject{
            {QStringLiteral("required"), true},
            {QStringLiteral("content"), QJsonObject{
                {QStringLiteral("application/json"), QJsonObject{
                    {QStringLiteral("schema"), schemaReference(schema)}}}}}};
    };
    const auto response = [&schemaReference](const QString &description,
                                              const QString &schema = {}) {
        QJsonObject result{{QStringLiteral("description"), description}};
        if (!schema.isEmpty()) {
            const QJsonObject content{
                {QStringLiteral("application/json"),
                 QJsonObject{{QStringLiteral("schema"), schemaReference(schema)}}}};
            result.insert(QStringLiteral("content"), content);
        }
        return result;
    };

    const QJsonObject successOrError{
        {QStringLiteral("200"), response(QStringLiteral("Operação concluída"),
                                         QStringLiteral("CommandResult"))},
        {QStringLiteral("400"), response(QStringLiteral("Payload inválido"),
                                         QStringLiteral("Error"))},
        {QStringLiteral("401"), response(QStringLiteral("Sessão inválida ou expirada"),
                                         QStringLiteral("Error"))},
        {QStringLiteral("403"), response(QStringLiteral("Comando ou origem não permitida"),
                                         QStringLiteral("Error"))},
        {QStringLiteral("413"), response(QStringLiteral("Payload acima de 64 KiB"),
                                         QStringLiteral("Error"))},
        {QStringLiteral("429"), response(QStringLiteral("Limite de comandos excedido"),
                                         QStringLiteral("Error"))},
    };

    const QJsonObject paths{
        {QStringLiteral("/api/v1/session"), QJsonObject{
            {QStringLiteral("post"), QJsonObject{
                {QStringLiteral("summary"), QStringLiteral("Cria uma sessão local")},
                {QStringLiteral("requestBody"), jsonBody(QStringLiteral("SessionRequest"))},
                {QStringLiteral("responses"), QJsonObject{
                    {QStringLiteral("201"), response(QStringLiteral("Sessão criada"),
                                                     QStringLiteral("SessionResponse"))},
                    {QStringLiteral("401"), response(QStringLiteral("Senha incorreta"),
                                                     QStringLiteral("Error"))},
                    {QStringLiteral("429"), response(QStringLiteral("Login temporariamente bloqueado"),
                                                     QStringLiteral("Error"))}}}}},
            {QStringLiteral("delete"), QJsonObject{
                {QStringLiteral("summary"), QStringLiteral("Revoga a sessão atual")},
                {QStringLiteral("security"), bearerSecurity},
                {QStringLiteral("responses"), QJsonObject{
                    {QStringLiteral("204"), response(QStringLiteral("Sessão revogada"))},
                    {QStringLiteral("401"), response(QStringLiteral("Sessão inválida"),
                                                     QStringLiteral("Error"))}}}}}}},
        {QStringLiteral("/api/v1/state"), QJsonObject{
            {QStringLiteral("get"), QJsonObject{
                {QStringLiteral("summary"), QStringLiteral("Retorna o snapshot operacional")},
                {QStringLiteral("security"), bearerSecurity},
                {QStringLiteral("responses"), QJsonObject{
                    {QStringLiteral("200"), response(QStringLiteral("Estado atual"),
                                                     QStringLiteral("StateSnapshot"))},
                    {QStringLiteral("401"), response(QStringLiteral("Sessão inválida"),
                                                     QStringLiteral("Error"))}}}}}}},
        {QStringLiteral("/api/v1/commands"), QJsonObject{
            {QStringLiteral("post"), QJsonObject{
                {QStringLiteral("summary"), QStringLiteral("Executa um comando permitido")},
                {QStringLiteral("security"), bearerSecurity},
                {QStringLiteral("requestBody"), jsonBody(QStringLiteral("CommandRequest"))},
                {QStringLiteral("responses"), successOrError}}}}},
        {QStringLiteral("/api/v1/health"), QJsonObject{
            {QStringLiteral("get"), QJsonObject{
                {QStringLiteral("summary"), QStringLiteral("Verifica o servidor local")},
                {QStringLiteral("responses"), QJsonObject{
                    {QStringLiteral("200"), response(QStringLiteral("Servidor disponível"),
                                                     QStringLiteral("Health"))}}}}}}},
        {QStringLiteral("/api/v1/openapi.json"), QJsonObject{
            {QStringLiteral("get"), QJsonObject{
                {QStringLiteral("summary"), QStringLiteral("Retorna este contrato OpenAPI")},
                {QStringLiteral("responses"), QJsonObject{
                    {QStringLiteral("200"), response(QStringLiteral("Documento OpenAPI"))}}}}}}},
        {QStringLiteral("/api/v1/ws"), QJsonObject{
            {QStringLiteral("get"), QJsonObject{
                {QStringLiteral("summary"), QStringLiteral("Canal WebSocket de estado e comandos")},
                {QStringLiteral("description"), QStringLiteral("A primeira mensagem em até cinco segundos deve ser {type: authenticate, token: ...}.")},
                {QStringLiteral("x-websocket"), true},
                {QStringLiteral("responses"), QJsonObject{
                    {QStringLiteral("101"), response(QStringLiteral("Protocolo atualizado para WebSocket"))}}}}}}},
    };

    QJsonObject schemas;
    schemas.insert(QStringLiteral("SessionRequest"), QJsonObject{
        {QStringLiteral("type"), QStringLiteral("object")},
        {QStringLiteral("required"), QJsonArray{QStringLiteral("password")}},
        {QStringLiteral("properties"), QJsonObject{
            {QStringLiteral("password"), QJsonObject{
                {QStringLiteral("type"), QStringLiteral("string")},
                {QStringLiteral("format"), QStringLiteral("password")}}}}}});
    schemas.insert(QStringLiteral("SessionResponse"), QJsonObject{
        {QStringLiteral("type"), QStringLiteral("object")},
        {QStringLiteral("required"), QJsonArray{QStringLiteral("accepted"),
                                                QStringLiteral("token"),
                                                QStringLiteral("expiresAt")}},
        {QStringLiteral("properties"), QJsonObject{
            {QStringLiteral("accepted"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}}},
            {QStringLiteral("token"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("expiresAt"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                                                      {QStringLiteral("format"), QStringLiteral("date-time")}}}}}});
    schemas.insert(QStringLiteral("CommandRequest"), QJsonObject{
        {QStringLiteral("type"), QStringLiteral("object")},
        {QStringLiteral("additionalProperties"), false},
        {QStringLiteral("required"), QJsonArray{QStringLiteral("id"),
                                                QStringLiteral("type"),
                                                QStringLiteral("payload")}},
        {QStringLiteral("properties"), QJsonObject{
            {QStringLiteral("id"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("type"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("payload"), QJsonObject{{QStringLiteral("type"), QStringLiteral("object")}}}}}});
    schemas.insert(QStringLiteral("CommandResult"), QJsonObject{
        {QStringLiteral("type"), QStringLiteral("object")},
        {QStringLiteral("required"), QJsonArray{QStringLiteral("accepted"),
                                                QStringLiteral("errorCode"),
                                                QStringLiteral("message"),
                                                QStringLiteral("stateRevision")}},
        {QStringLiteral("properties"), QJsonObject{
            {QStringLiteral("accepted"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}}},
            {QStringLiteral("errorCode"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("message"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("stateRevision"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")},
                                                           {QStringLiteral("format"), QStringLiteral("uint64")}}}}}});
    schemas.insert(QStringLiteral("StateSnapshot"), QJsonObject{
        {QStringLiteral("type"), QStringLiteral("object")},
        {QStringLiteral("required"), QJsonArray{QStringLiteral("stateRevision")}},
        {QStringLiteral("additionalProperties"), true}});
    schemas.insert(QStringLiteral("Health"), QJsonObject{
        {QStringLiteral("type"), QStringLiteral("object")},
        {QStringLiteral("required"), QJsonArray{QStringLiteral("ok"),
                                                QStringLiteral("service"),
                                                QStringLiteral("apiVersion")}}});
    schemas.insert(QStringLiteral("Error"), QJsonObject{
        {QStringLiteral("type"), QStringLiteral("object")},
        {QStringLiteral("required"), QJsonArray{QStringLiteral("accepted"),
                                                QStringLiteral("errorCode"),
                                                QStringLiteral("message")}}});

    return {
        {QStringLiteral("openapi"), QStringLiteral("3.1.0")},
        {QStringLiteral("info"), QJsonObject{
            {QStringLiteral("title"), QStringLiteral("HolyScreen Local API")},
            {QStringLiteral("version"), QStringLiteral("v1")},
            {QStringLiteral("description"), QStringLiteral("API offline para a rede local. Não exponha diretamente à internet.")}}},
        {QStringLiteral("paths"), paths},
        {QStringLiteral("components"), QJsonObject{
            {QStringLiteral("securitySchemes"), QJsonObject{
                {QStringLiteral("bearerAuth"), QJsonObject{
                    {QStringLiteral("type"), QStringLiteral("http")},
                    {QStringLiteral("scheme"), QStringLiteral("bearer")},
                    {QStringLiteral("bearerFormat"), QStringLiteral("opaque-session-token")}}}}},
            {QStringLiteral("schemas"), schemas}}},
    };
}

QByteArray LocalApiServer::remotePage()
{
    return QByteArrayLiteral(R"HTML(<!doctype html>
<html lang="pt-BR"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="theme-color" content="#08111f"><link rel="manifest" href="/manifest.webmanifest"><title>HolyScreen Remoto</title>
<style>:root{color-scheme:dark}*{box-sizing:border-box}body{margin:0;background:#08111f;color:#edf6ff;font:15px system-ui}main{max-width:820px;margin:auto;padding:18px}.top{display:flex;align-items:center;gap:12px;justify-content:space-between}.card{background:#101d31;padding:16px;border:1px solid #243b5c;border-radius:16px;margin:12px 0}.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.three{grid-template-columns:repeat(3,minmax(0,1fr))}button,input,select,textarea{font:inherit;border:0;border-radius:10px;padding:13px;background:#152137;color:white;width:100%}button{background:#183b69;font-weight:700;cursor:pointer}button.primary{background:#2563eb}button.danger{background:#a51d2d}button.small{padding:8px;width:auto}textarea{min-height:70px;resize:vertical}label{display:block;color:#9fb0c7;margin-top:10px}.state{padding:12px;background:#091525;border-radius:10px;white-space:pre-wrap}.muted{color:#9fb0c7}.ok{color:#70e1a7}.hidden{display:none}details summary{cursor:pointer;font-weight:800;font-size:17px;padding:5px 0}.list button{margin-top:7px;text-align:left}.row{display:flex;gap:9px;align-items:center}.row>*{flex:1}@media(max-width:520px){.grid,.three{grid-template-columns:1fr}.top{align-items:flex-start;flex-direction:column}.row{flex-direction:column}}</style></head>
<body><main><div class="top"><div><h1>HOLYSCREEN</h1><div id="status" class="muted">Autentique para controlar</div></div><button id="logoutButton" class="small hidden" onclick="logout()">SAIR</button></div>
<section id="login" class="card"><label for="password">Senha do controle remoto</label><input id="password" type="password" autocomplete="current-password"><button class="primary" onclick="authenticate()">ENTRAR</button></section>
<div id="controls" class="hidden"><section class="card"><div id="presentationState" class="state">Nenhuma apresentação ativa</div><div class="grid three"><button onclick="send('presentation.slide.first',{})">PRIMEIRO</button><button onclick="send('presentation.slide.previous',{})">◀ ANTERIOR</button><button class="primary" onclick="send('presentation.slide.next',{})">PRÓXIMO ▶</button><button onclick="send('presentation.slide.last',{})">ÚLTIMO</button><button onclick="send('presentation.stop',{})">OCULTAR</button><button id="blackout" class="danger" onclick="toggleBlackout()">BLACKOUT</button></div></section>
<details class="card" open><summary>Player e playlist</summary><div id="mediaState" class="state">Player parado</div><div class="grid three"><button onclick="send('media.previous',{})">◀ MÍDIA</button><button onclick="send('media.pause.toggle',{})">TOCAR / PAUSAR</button><button onclick="send('media.next',{})">MÍDIA ▶</button></div><button class="danger" onclick="send('media.stop',{})">PARAR</button><label>Repetição</label><select id="repeat" onchange="send('media.repeat.set',{mode:this.value})"><option value="off">Desativada</option><option value="one">Repetir item</option><option value="all">Repetir playlist</option></select><div id="playlist" class="list"></div></details>
<details class="card"><summary>Bíblia</summary><div class="grid three"><input id="book" inputmode="numeric" placeholder="Livro (1–66)"><input id="chapter" inputmode="numeric" placeholder="Capítulo"><input id="verse" inputmode="numeric" placeholder="Versículo"></div><button class="primary" onclick="presentBible()">APRESENTAR REFERÊNCIA</button><div id="bibleState" class="state"></div></details>
<details class="card"><summary>Eventos</summary><label>Evento</label><select id="eventPicker" onchange="send('event.select',{eventId:this.value})"></select><div id="eventItems" class="list"></div></details>
<details class="card"><summary>Palco e overlays</summary><label>Mensagem ao palco</label><textarea id="stage"></textarea><button onclick="send('stage.message.set',{message:byId('stage').value})">ENVIAR AO PALCO</button><label>Mensagem ao público</label><textarea id="audience"></textarea><button onclick="send('overlay.audience-message.set',{message:byId('audience').value})">EXIBIR MENSAGEM</button><label>Alerta</label><textarea id="alert"></textarea><button onclick="send('overlay.alert.set',{message:byId('alert').value})">EXIBIR ALERTA</button><div class="row"><input id="lowerTitle" placeholder="Título"><input id="lowerSubtitle" placeholder="Subtítulo"></div><button onclick="send('overlay.lower-third.set',{title:byId('lowerTitle').value,subtitle:byId('lowerSubtitle').value})">EXIBIR LOWER THIRD</button></details>
<details class="card"><summary>Timers</summary><div id="timerState" class="state"></div><div class="row"><input id="countdown" type="number" min="1" value="300" placeholder="Segundos"><button onclick="send('timer.countdown.start',{seconds:Number(byId('countdown').value)})">INICIAR CONTAGEM</button><button onclick="send('timer.countdown.stop',{})">PARAR CONTAGEM</button></div><div class="grid three"><button onclick="send('timer.stopwatch.start',{})">INICIAR CRONÔMETRO</button><button onclick="send('timer.stopwatch.pause',{})">PAUSAR</button><button onclick="send('timer.stopwatch.reset',{})">ZERAR</button></div></details></div></main>
<script>
let token=sessionStorage.getItem('hs-token')||'',ws=null,state={},reconnectTimer;
const byId=id=>document.getElementById(id);
async function authenticate(){const r=await fetch('/api/v1/session',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({password:byId('password').value})});const d=await r.json();if(!d.accepted){byId('status').textContent=d.errorCode==='login_blocked'?'Acesso temporariamente bloqueado':'Senha incorreta';return}token=d.token;sessionStorage.setItem('hs-token',token);showControls();connect()}
function showControls(){byId('login').classList.add('hidden');byId('controls').classList.remove('hidden');byId('logoutButton').classList.remove('hidden')}
function showLogin(message='Autentique para controlar'){token='';sessionStorage.removeItem('hs-token');byId('login').classList.remove('hidden');byId('controls').classList.add('hidden');byId('logoutButton').classList.add('hidden');byId('status').textContent=message}
async function logout(){if(token)await fetch('/api/v1/session',{method:'DELETE',headers:{Authorization:'Bearer '+token}});if(ws)ws.close();showLogin()}
function connect(){clearTimeout(reconnectTimer);ws=new WebSocket(`${location.protocol==='https:'?'wss':'ws'}://${location.host}/api/v1/ws`);ws.onopen=()=>ws.send(JSON.stringify({type:'authenticate',token}));ws.onmessage=e=>{const d=JSON.parse(e.data);if(d.type==='authenticated'){byId('status').textContent='Conectado';byId('status').className='ok'}if(d.type==='state'){state=d.data||{};render()}if(d.type==='commandResult'&&!d.accepted)byId('status').textContent=d.message||d.errorCode};ws.onclose=e=>{if(e.reason==='invalid_session'||e.reason==='sessions_revoked'||e.reason==='authentication_required'){showLogin('Sessão encerrada');return}byId('status').textContent='Reconectando…';reconnectTimer=setTimeout(connect,1200)}}
function commandId(){return crypto.randomUUID?crypto.randomUUID():Date.now()+'-'+Math.random()}
function send(type,payload){if(!ws||ws.readyState!==1){byId('status').textContent='Sem conexão';return}ws.send(JSON.stringify({type:'command',command:{id:commandId(),type,payload}}))}
function toggleBlackout(){send('presentation.blackout.set',{enabled:!state.blackout})}
function presentBible(){send('bible.reference.present',{bookId:Number(byId('book').value),chapter:Number(byId('chapter').value),verse:Number(byId('verse').value)})}
function render(){const p=state.presentation||{},m=state.media||{},b=state.bible||{},ev=state.event||{},timers=state.timers||{},overlays=state.overlays||{};byId('presentationState').textContent=(p.title||'Sem apresentação')+'\nAtual: '+(p.slideText||'—')+'\nPróximo: '+(p.nextSlideText||'—');byId('mediaState').textContent=(m.title||'Nenhuma mídia')+' • '+(m.state||'stopped');byId('repeat').value=m.repeatMode||'off';byId('blackout').textContent=state.blackout?'DESATIVAR BLACKOUT':'BLACKOUT';byId('playlist').innerHTML='';(m.playlist||[]).forEach(item=>{const button=document.createElement('button');button.textContent='▶ '+item.title;button.onclick=()=>send('media.play',{mediaId:item.id});byId('playlist').appendChild(button)});byId('bibleState').textContent=b.reference||'Nenhuma referência';const selected=ev.id||'';byId('eventPicker').innerHTML='<option value="">Selecione…</option>';(ev.events||[]).forEach(item=>byId('eventPicker').add(new Option(item.title,item.id,item.id===selected,item.id===selected)));byId('eventItems').innerHTML='';(ev.items||[]).forEach(item=>{const button=document.createElement('button');button.textContent='EXECUTAR • '+item.title;button.onclick=()=>send('event.item.execute',{itemId:item.id});byId('eventItems').appendChild(button)});byId('stage').value=(state.stage||{}).message||'';byId('audience').value=overlays.audienceMessage||'';byId('alert').value=overlays.alertMessage||'';byId('lowerTitle').value=overlays.lowerThirdTitle||'';byId('lowerSubtitle').value=overlays.lowerThirdSubtitle||'';byId('timerState').textContent='Contagem: '+(timers.countdownText||'00:00')+' • Cronômetro: '+(timers.stopwatchText||'00:00')}
if('serviceWorker'in navigator)navigator.serviceWorker.register('/sw.js');if(token){showControls();connect()}
</script></body></html>)HTML");
}

QByteArray LocalApiServer::manifest()
{
    return QByteArrayLiteral(R"JSON({"name":"HolyScreen Remoto","short_name":"HolyScreen","start_url":"/","display":"standalone","background_color":"#08111f","theme_color":"#08111f","lang":"pt-BR"})JSON");
}

QByteArray LocalApiServer::serviceWorker()
{
    return QByteArrayLiteral(R"JS(const CACHE='holyscreen-remote-v1';self.addEventListener('install',e=>e.waitUntil(caches.open(CACHE).then(c=>c.addAll(['/','/manifest.webmanifest']))));self.addEventListener('activate',e=>e.waitUntil(self.clients.claim()));self.addEventListener('fetch',e=>{if(e.request.method==='GET')e.respondWith(fetch(e.request).catch(()=>caches.match(e.request)))})JS");
}

} // namespace churchpresenter
