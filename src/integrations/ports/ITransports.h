#pragma once

#include <QByteArray>
#include <QHostAddress>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include <functional>

namespace churchpresenter {

struct HttpRequest {
    QString method;
    QString url;
    QVariantMap headers;
    QByteArray body;
    int timeoutMs = 5000;
    qint64 maximumResponseBytes = 512 * 1024;
};

struct HttpResponse {
    bool completed = false;
    int status = 0;
    QString errorCode;
    QString message;
    QVariantMap headers;
    QByteArray body;
};

//! Porta HTTP de saída. Só `http` e `https` são aceitos pelo adapter.
class IHttpTransport {
public:
    using Completion = std::function<void(const HttpResponse &)>;

    virtual ~IHttpTransport() = default;
    virtual void send(const HttpRequest &request, Completion completion) = 0;
    virtual void cancelAll() = 0;
};

class IWebSocketTransport {
public:
    using StateChanged = std::function<void(const QString &state, const QString &message)>;
    using Completion = std::function<void(bool accepted, const QString &errorCode,
                                          const QString &message)>;

    virtual ~IWebSocketTransport() = default;
    virtual void connectTo(const QString &url, int timeoutMs, Completion completion) = 0;
    virtual void sendText(const QString &url, const QString &text, Completion completion) = 0;
    virtual void disconnectFrom(const QString &url) = 0;
    [[nodiscard]] virtual QString state(const QString &url) const = 0;
    virtual void setStateObserver(StateChanged observer) = 0;
    virtual void cancelAll() = 0;
};

struct MidiMessage {
    int channel = 0;
    int status = 0;
    int data1 = 0;
    int data2 = 0;
};

class IMidiTransport {
public:
    virtual ~IMidiTransport() = default;
    [[nodiscard]] virtual QStringList outputPorts() const = 0;
    virtual bool openPort(const QString &portName) = 0;
    virtual bool send(const QString &portName, const MidiMessage &message) = 0;
    virtual void closeAll() = 0;
    [[nodiscard]] virtual QString lastError() const = 0;
};

class IOscTransport {
public:
    virtual ~IOscTransport() = default;
    virtual bool send(const QString &host, quint16 port, const QByteArray &datagram) = 0;
    [[nodiscard]] virtual QString lastError() const = 0;
};

struct ProcessRequest {
    QString executable;
    QStringList arguments;
    QString workingDirectory;
    QVariantMap environment;
    int timeoutMs = 5000;
    qint64 maximumOutputBytes = 64 * 1024;
};

struct ProcessResult {
    bool started = false;
    bool finished = false;
    int exitCode = -1;
    QString errorCode;
    QString message;
    QByteArray standardOutput;
    QByteArray standardError;
};

//! Porta de execução de processo externo. A allowlist e a confirmação ficam na
//! camada de aplicação; o transporte nunca usa shell.
class IProcessRunner {
public:
    using Completion = std::function<void(const ProcessResult &)>;

    virtual ~IProcessRunner() = default;
    virtual void run(const ProcessRequest &request, Completion completion) = 0;
    virtual void cancelAll() = 0;
};

} // namespace churchpresenter
