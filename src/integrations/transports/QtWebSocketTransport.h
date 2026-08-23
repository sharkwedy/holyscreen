#pragma once

#include "integrations/ports/ITransports.h"

#include <QHash>
#include <QObject>
#include <QString>

QT_BEGIN_NAMESPACE
class QWebSocket;
class QTimer;
QT_END_NAMESPACE

namespace churchpresenter {

//! Cliente WebSocket com reconexão de backoff limitado, estado observável e
//! fila de mensagens com teto.
class QtWebSocketTransport final : public QObject, public IWebSocketTransport {
    Q_OBJECT

public:
    static constexpr int MaximumReconnectAttempts = 5;
    static constexpr int BaseBackoffMs = 500;
    static constexpr int MaximumBackoffMs = 8000;
    static constexpr int MaximumQueuedMessages = 32;

    explicit QtWebSocketTransport(QObject *parent = nullptr);
    ~QtWebSocketTransport() override;

    void connectTo(const QString &url, int timeoutMs, Completion completion) override;
    void sendText(const QString &url, const QString &text, Completion completion) override;
    void disconnectFrom(const QString &url) override;
    [[nodiscard]] QString state(const QString &url) const override;
    void setStateObserver(StateChanged observer) override;
    void cancelAll() override;

    //! Reconexão automática pode ser desligada nos testes determinísticos.
    void setAutomaticReconnect(bool enabled);
    [[nodiscard]] int queuedMessages(const QString &url) const;

private:
    struct Connection;

    Connection *connectionFor(const QString &url);
    void publishState(const QString &url, const QString &state, const QString &message = {});
    void scheduleReconnect(const QString &url);
    void flushQueue(Connection &connection);

    QHash<QString, Connection *> m_connections;
    StateChanged m_observer;
    bool m_automaticReconnect = true;
};

} // namespace churchpresenter
