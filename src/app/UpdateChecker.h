#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QUrl>

namespace churchpresenter {

struct UpdateRelease
{
    QString latestVersion;
    QUrl releaseUrl;
    QUrl downloadUrl;
    QString assetName;
    QString sha256;
    qint64 assetSize = 0;
    bool available = false;
    QString error;
};

class UpdateChecker final : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);

    [[nodiscard]] static QUrl defaultEndpoint();
    [[nodiscard]] static UpdateRelease parseGitHubRelease(
        const QByteArray &payload,
        const QString &currentVersion,
        const QString &platform);

    void check(const QUrl &endpoint, const QString &currentVersion);

signals:
    void completed(QString latestVersion, QUrl downloadUrl, bool available, QString error);

private:
    QNetworkAccessManager m_network;
};

} // namespace churchpresenter
