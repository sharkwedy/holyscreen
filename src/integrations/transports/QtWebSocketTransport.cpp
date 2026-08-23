#include "integrations/transports/QtWebSocketTransport.h"

#include <QTimer>
#include <QUrl>
#include <QWebSocket>

#include <algorithm>

namespace churchpresenter {

struct QtWebSocketTransport::Connection {
    QWebSocket *socket = nullptr;
    QTimer *connectTimeout = nullptr;
    QTimer *reconnect = nullptr;
    QString state = QStringLiteral("disconnected");
    int attempts = 0;
    QList<QString> queue;
    QList<Completion> pendingConnections;
    bool closedByUser = false;
};

QtWebSocketTransport::QtWebSocketTransport(QObject *parent)
    : QObject(parent)
{
}

QtWebSocketTransport::~QtWebSocketTransport()
{
    cancelAll();
    qDeleteAll(m_connections);
    m_connections.clear();
}

void QtWebSocketTransport::setAutomaticReconnect(bool enabled)
{
    m_automaticReconnect = enabled;
}

void QtWebSocketTransport::setStateObserver(StateChanged observer)
{
    m_observer = std::move(observer);
}

QString QtWebSocketTransport::state(const QString &url) const
{
    const auto found = m_connections.constFind(url);
    return found == m_connections.cend() ? QStringLiteral("disconnected") : (*found)->state;
}

int QtWebSocketTransport::queuedMessages(const QString &url) const
{
    const auto found = m_connections.constFind(url);
    return found == m_connections.cend() ? 0 : static_cast<int>((*found)->queue.size());
}

void QtWebSocketTransport::publishState(const QString &url, const QString &state,
                                        const QString &message)
{
    if (auto *connection = m_connections.value(url, nullptr)) connection->state = state;
    if (m_observer) m_observer(state, message.isEmpty() ? url : message);
}

QtWebSocketTransport::Connection *QtWebSocketTransport::connectionFor(const QString &url)
{
    if (auto *existing = m_connections.value(url, nullptr)) return existing;

    auto *connection = new Connection;
    connection->socket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    connection->connectTimeout = new QTimer(this);
    connection->connectTimeout->setSingleShot(true);
    connection->reconnect = new QTimer(this);
    connection->reconnect->setSingleShot(true);
    m_connections.insert(url, connection);

    connect(connection->socket, &QWebSocket::connected, this, [this, url] {
        auto *current = m_connections.value(url, nullptr);
        if (!current) return;
        current->connectTimeout->stop();
        current->attempts = 0;
        publishState(url, QStringLiteral("connected"));
        const auto pending = current->pendingConnections;
        current->pendingConnections.clear();
        for (const auto &completion : pending) {
            if (completion) completion(true, {}, QStringLiteral("Conectado."));
        }
        flushQueue(*current);
    });

    connect(connection->socket, &QWebSocket::disconnected, this, [this, url] {
        auto *current = m_connections.value(url, nullptr);
        if (!current) return;
        current->connectTimeout->stop();
        publishState(url, QStringLiteral("disconnected"));
        if (!current->closedByUser) scheduleReconnect(url);
    });

    connect(connection->socket, &QWebSocket::errorOccurred, this,
            [this, url](QAbstractSocket::SocketError) {
        auto *current = m_connections.value(url, nullptr);
        if (!current) return;
        current->connectTimeout->stop();
        const auto message = current->socket->errorString();
        publishState(url, QStringLiteral("error"), message);
        const auto pending = current->pendingConnections;
        current->pendingConnections.clear();
        for (const auto &completion : pending) {
            if (completion) completion(false, QStringLiteral("connection_failed"), message);
        }
        if (!current->closedByUser) scheduleReconnect(url);
    });

    connect(connection->connectTimeout, &QTimer::timeout, this, [this, url] {
        auto *current = m_connections.value(url, nullptr);
        if (!current || current->socket->state() == QAbstractSocket::ConnectedState) return;
        current->socket->abort();
        publishState(url, QStringLiteral("error"), QStringLiteral("Tempo de conexão esgotado."));
        const auto pending = current->pendingConnections;
        current->pendingConnections.clear();
        for (const auto &completion : pending) {
            if (completion) {
                completion(false, QStringLiteral("timeout"),
                           QStringLiteral("Tempo de conexão esgotado."));
            }
        }
    });

    connect(connection->reconnect, &QTimer::timeout, this, [this, url] {
        auto *current = m_connections.value(url, nullptr);
        if (!current || current->closedByUser) return;
        if (current->socket->state() != QAbstractSocket::UnconnectedState) return;
        publishState(url, QStringLiteral("connecting"));
        current->socket->open(QUrl(url));
    });

    return connection;
}

void QtWebSocketTransport::scheduleReconnect(const QString &url)
{
    auto *connection = m_connections.value(url, nullptr);
    if (!connection || !m_automaticReconnect) return;
    if (connection->attempts >= MaximumReconnectAttempts) {
        publishState(url, QStringLiteral("unavailable"),
                     QStringLiteral("Limite de reconexões atingido."));
        return;
    }
    ++connection->attempts;
    const auto delay = std::min(MaximumBackoffMs, BaseBackoffMs * (1 << (connection->attempts - 1)));
    connection->reconnect->start(delay);
}

void QtWebSocketTransport::flushQueue(Connection &connection)
{
    while (!connection.queue.isEmpty()
           && connection.socket->state() == QAbstractSocket::ConnectedState) {
        connection.socket->sendTextMessage(connection.queue.takeFirst());
    }
    connection.socket->flush();
}

void QtWebSocketTransport::connectTo(const QString &url, int timeoutMs, Completion completion)
{
    const QUrl parsed(url);
    if (!parsed.isValid() || (parsed.scheme() != QStringLiteral("ws")
                              && parsed.scheme() != QStringLiteral("wss"))) {
        if (completion) {
            completion(false, QStringLiteral("invalid_url"),
                       QStringLiteral("Somente ws e wss são aceitos."));
        }
        return;
    }
    auto *connection = connectionFor(url);
    connection->closedByUser = false;
    if (connection->socket->state() == QAbstractSocket::ConnectedState) {
        if (completion) completion(true, {}, QStringLiteral("Já conectado."));
        return;
    }
    if (completion) connection->pendingConnections.append(completion);
    if (connection->socket->state() == QAbstractSocket::UnconnectedState) {
        publishState(url, QStringLiteral("connecting"));
        connection->socket->open(parsed);
    }
    connection->connectTimeout->start(std::max(250, timeoutMs));
}

void QtWebSocketTransport::sendText(const QString &url, const QString &text, Completion completion)
{
    auto *connection = connectionFor(url);
    if (connection->socket->state() == QAbstractSocket::ConnectedState) {
        connection->socket->sendTextMessage(text);
        connection->socket->flush();
        if (completion) completion(true, {}, QStringLiteral("Mensagem enviada."));
        return;
    }
    if (connection->queue.size() >= MaximumQueuedMessages) {
        if (completion) {
            completion(false, QStringLiteral("queue_full"),
                       QStringLiteral("A fila de mensagens está cheia."));
        }
        return;
    }
    connection->queue.append(text);
    connectTo(url, 5000, [completion](bool accepted, const QString &errorCode,
                                      const QString &message) {
        if (!completion) return;
        if (accepted) {
            completion(true, {}, QStringLiteral("Mensagem enviada após conectar."));
        } else {
            completion(false, errorCode, message);
        }
    });
}

void QtWebSocketTransport::disconnectFrom(const QString &url)
{
    auto *connection = m_connections.value(url, nullptr);
    if (!connection) return;
    connection->closedByUser = true;
    connection->reconnect->stop();
    connection->connectTimeout->stop();
    connection->queue.clear();
    connection->socket->close();
    publishState(url, QStringLiteral("disconnected"));
}

void QtWebSocketTransport::cancelAll()
{
    for (auto it = m_connections.cbegin(); it != m_connections.cend(); ++it) {
        auto *connection = it.value();
        connection->closedByUser = true;
        connection->reconnect->stop();
        connection->connectTimeout->stop();
        connection->queue.clear();
        connection->pendingConnections.clear();
        connection->socket->abort();
        connection->state = QStringLiteral("disconnected");
    }
}

} // namespace churchpresenter
