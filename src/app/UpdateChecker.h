#pragma once

#include <QMetaType>
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

    //! Verdadeiro quando \a url é um endereço HTTPS do GitHub sem credenciais
    //! embutidas. O download da atualização usa a mesma regra do manifesto.
    [[nodiscard]] static bool isTrustedDownloadUrl(const QUrl &url);

signals:
    //! Entrega a release inteira, incluindo o pacote da plataforma, o tamanho e
    //! o SHA-256 publicado pelo GitHub, para que o download possa ser conferido.
    void completed(const churchpresenter::UpdateRelease &release);

private:
    QNetworkAccessManager m_network;
};

} // namespace churchpresenter

Q_DECLARE_METATYPE(churchpresenter::UpdateRelease)
