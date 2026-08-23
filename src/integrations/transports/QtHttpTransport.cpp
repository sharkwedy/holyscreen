#include "integrations/transports/QtHttpTransport.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QTimer>
#include <QUrl>

namespace churchpresenter {
namespace {

QString errorCodeFor(QNetworkReply::NetworkError error)
{
    switch (error) {
    case QNetworkReply::TimeoutError:
    case QNetworkReply::OperationCanceledError:
        return QStringLiteral("timeout");
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::NetworkSessionFailedError:
    case QNetworkReply::TemporaryNetworkFailureError:
        return QStringLiteral("connection_failed");
    case QNetworkReply::SslHandshakeFailedError:
        return QStringLiteral("tls_failed");
    default:
        return QStringLiteral("request_failed");
    }
}

} // namespace

QtHttpTransport::QtHttpTransport(QObject *parent)
    : QObject(parent)
{
    m_network.setTransferTimeout(0);
    m_network.setAutoDeleteReplies(false);
}

QtHttpTransport::~QtHttpTransport()
{
    cancelAll();
}

void QtHttpTransport::send(const HttpRequest &request, Completion completion)
{
    const QUrl url(request.url);
    if (!url.isValid() || (url.scheme() != QStringLiteral("http")
                           && url.scheme() != QStringLiteral("https"))) {
        completion(HttpResponse{.completed = false,
                                .errorCode = QStringLiteral("invalid_url"),
                                .message = QStringLiteral("Somente http e https são aceitos.")});
        return;
    }

    QNetworkRequest networkRequest(url);
    networkRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                QNetworkRequest::SameOriginRedirectPolicy);
    networkRequest.setMaximumRedirectsAllowed(MaximumRedirects);
    networkRequest.setTransferTimeout(request.timeoutMs);
    // TLS permanece na verificação padrão do Qt: nunca ignoramos certificados.
    networkRequest.setSslConfiguration(QSslConfiguration::defaultConfiguration());
    for (auto it = request.headers.cbegin(); it != request.headers.cend(); ++it) {
        networkRequest.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
    }

    const auto method = request.method.toUpper().toUtf8();
    QNetworkReply *reply = nullptr;
    if (method == QByteArrayLiteral("GET")) {
        reply = m_network.get(networkRequest);
    } else if (method == QByteArrayLiteral("HEAD")) {
        reply = m_network.head(networkRequest);
    } else if (method == QByteArrayLiteral("POST")) {
        reply = m_network.post(networkRequest, request.body);
    } else if (method == QByteArrayLiteral("PUT")) {
        reply = m_network.put(networkRequest, request.body);
    } else {
        reply = m_network.sendCustomRequest(networkRequest, method, request.body);
    }
    if (!reply) {
        completion(HttpResponse{.completed = false,
                                .errorCode = QStringLiteral("request_failed"),
                                .message = QStringLiteral("A requisição não pôde ser criada.")});
        return;
    }
    m_pending.append(reply);

    const auto maximumBytes = request.maximumResponseBytes;
    connect(reply, &QNetworkReply::downloadProgress, reply,
            [reply, maximumBytes](qint64 received, qint64) {
        if (maximumBytes > 0 && received > maximumBytes) reply->abort();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, completion, maximumBytes] {
        m_pending.removeAll(reply);
        HttpResponse response;
        response.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        for (const auto &pair : reply->rawHeaderPairs()) {
            response.headers.insert(QString::fromUtf8(pair.first),
                                    QString::fromUtf8(pair.second));
        }
        const auto body = reply->readAll();
        response.body = maximumBytes > 0 ? body.left(maximumBytes) : body;
        if (reply->error() != QNetworkReply::NoError && response.status == 0) {
            response.completed = false;
            response.errorCode = errorCodeFor(reply->error());
            response.message = reply->errorString();
        } else {
            response.completed = true;
        }
        reply->deleteLater();
        completion(response);
    });
}

void QtHttpTransport::cancelAll()
{
    const auto pending = m_pending;
    m_pending.clear();
    for (const auto &reply : pending) {
        if (reply) reply->abort();
    }
}

} // namespace churchpresenter
