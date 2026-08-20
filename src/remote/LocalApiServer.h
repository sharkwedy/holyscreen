#pragma once

#include <QHttpServer>
#include <QJsonObject>
#include <QTcpServer>
#include <QVector>
#include <functional>

QT_BEGIN_NAMESPACE
class QWebSocket;
QT_END_NAMESPACE

namespace churchpresenter {

class LocalApiServer final : public QObject {
    Q_OBJECT
public:
    explicit LocalApiServer(QObject *parent = nullptr);
    ~LocalApiServer() override;

    bool start(quint16 port = 43120);
    void stop();
    [[nodiscard]] bool running() const;
    [[nodiscard]] quint16 port() const;
    [[nodiscard]] QString localAddress() const;
    [[nodiscard]] QString remoteUrl() const;
    [[nodiscard]] int clientCount() const;
    [[nodiscard]] QString lastError() const;
    void setStateProvider(std::function<QJsonObject()> provider);
    bool processCommandPayload(const QByteArray &payload);
    void broadcastState();
    [[nodiscard]] static QByteArray remotePage();

signals:
    void commandReceived(const QString &command, const QJsonObject &arguments);
    void statusChanged();

private:
    QJsonObject state() const;
    void configureRoutes();
    QHttpServer m_http;
    QTcpServer m_tcp;
    QVector<QWebSocket *> m_clients;
    std::function<QJsonObject()> m_stateProvider;
    QString m_lastError;
};

} // namespace churchpresenter
