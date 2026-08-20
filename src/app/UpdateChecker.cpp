#include "app/UpdateChecker.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QVersionNumber>

namespace churchpresenter {

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
{
}

void UpdateChecker::check(const QUrl &url, const QString &currentVersion)
{
    if (!url.isValid() || url.isEmpty()) {
        emit completed({}, {}, false, QStringLiteral("Endpoint de atualização não configurado."));
        return;
    }

    auto *reply = m_network.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, currentVersion] {
        const auto error = reply->error() == QNetworkReply::NoError
            ? QString{}
            : reply->errorString();
        const auto object = QJsonDocument::fromJson(reply->readAll()).object();
        const auto latest = object.value(QStringLiteral("version")).toString();
        const QUrl download(object.value(QStringLiteral("url")).toString());
        const bool available = error.isEmpty()
            && QVersionNumber::fromString(latest) > QVersionNumber::fromString(currentVersion);

        reply->deleteLater();
        emit completed(latest, download, available, error);
    });
}

} // namespace churchpresenter
