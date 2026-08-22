#pragma once

#include "core/CommandTypes.h"
#include "core/EventTypes.h"
#include "remote/RemoteAuthManager.h"

#include <QHash>
#include <QHostAddress>
#include <QHttpServer>
#include <QJsonObject>
#include <QTcpServer>

#include <functional>

QT_BEGIN_NAMESPACE
class QWebSocket;
QT_END_NAMESPACE

namespace churchpresenter {

class LocalApiServer final : public QObject {
    Q_OBJECT
public:
    static constexpr qsizetype MaximumPayloadBytes = 64 * 1024;

    explicit LocalApiServer(QObject *parent = nullptr);
    ~LocalApiServer() override;

    bool start(quint16 port = 43120,
               const QHostAddress &address = QHostAddress::AnyIPv4);
    void stop();
    [[nodiscard]] bool running() const;
    [[nodiscard]] quint16 port() const;
    [[nodiscard]] QString localAddress() const;
    [[nodiscard]] QString remoteUrl() const;
    [[nodiscard]] int clientCount() const;
    [[nodiscard]] int sessionCount() const;
    [[nodiscard]] QString lastError() const;

    void setStateProvider(std::function<QJsonObject()> provider);
    void setCommandDispatcher(std::function<CommandResult(const Command &)> dispatcher);
    [[nodiscard]] QVariantMap setPassword(const QString &password);
    bool loadCredentials(const QVariantMap &credentials);
    [[nodiscard]] bool hasPassword() const;
    void revokeAllSessions();
    void broadcastState();
    void broadcastEvent(const DomainEvent &event, quint64 stateRevision);

    [[nodiscard]] static QJsonObject openApiDocument();
    [[nodiscard]] static QByteArray remotePage();
    [[nodiscard]] static QByteArray manifest();
    [[nodiscard]] static QByteArray serviceWorker();

signals:
    void statusChanged();

private:
    QJsonObject state() const;
    void configureRoutes();
    [[nodiscard]] bool sameOrigin(const QHttpServerRequest &request) const;
    [[nodiscard]] QString bearerToken(const QHttpServerRequest &request) const;
    CommandResult processCommand(const QJsonObject &object, const QByteArray &sessionHash,
                                 const QString &source);
    void sendWebSocketJson(QWebSocket *socket, const QJsonObject &object);

    QHttpServer m_http;
    QTcpServer m_tcp;
    QHash<QWebSocket *, QByteArray> m_clients;
    std::function<QJsonObject()> m_stateProvider;
    std::function<CommandResult(const Command &)> m_commandDispatcher;
    RemoteAuthManager m_auth;
    QString m_lastError;
    quint64 m_lastStateRevision = 0;
};

} // namespace churchpresenter
