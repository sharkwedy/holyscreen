#include "app/UpdateChecker.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <memory>

namespace churchpresenter {

namespace {
constexpr qsizetype maximumManifestSize = 256 * 1024;
constexpr qint64 maximumAssetSize = 10LL * 1024 * 1024 * 1024;

struct SemanticVersion
{
    int major = 0;
    int minor = 0;
    int patch = 0;
    int rc = -1;
    bool valid = false;
};

SemanticVersion parseVersion(QString version)
{
    static const QRegularExpression expression(
        QStringLiteral(R"(^v?(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-rc\.(0|[1-9]\d*))?$)"));
    const auto match = expression.match(version.trimmed());
    if (!match.hasMatch()) return {};
    return {
        match.captured(1).toInt(),
        match.captured(2).toInt(),
        match.captured(3).toInt(),
        match.captured(4).isEmpty() ? -1 : match.captured(4).toInt(),
        true
    };
}

bool newerThan(const SemanticVersion &candidate, const SemanticVersion &current)
{
    if (!candidate.valid || !current.valid) return false;
    if (candidate.major != current.major) return candidate.major > current.major;
    if (candidate.minor != current.minor) return candidate.minor > current.minor;
    if (candidate.patch != current.patch) return candidate.patch > current.patch;
    if (candidate.rc == current.rc) return false;
    if (candidate.rc < 0) return true;
    if (current.rc < 0) return false;
    return candidate.rc > current.rc;
}

QString runtimePlatform()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macos");
#else
    return QStringLiteral("linux");
#endif
}

int assetPriority(const QString &name, const QString &platform)
{
    const auto lower = name.toLower();
    if (platform == QStringLiteral("windows")) {
        if (lower.endsWith(QStringLiteral(".exe"))) return 0;
        if (lower.endsWith(QStringLiteral(".zip"))) return 1;
    } else if (platform == QStringLiteral("macos")) {
        if (lower.endsWith(QStringLiteral(".dmg"))) return 0;
        if (lower.endsWith(QStringLiteral(".zip"))) return 1;
    } else if (platform == QStringLiteral("linux")) {
        if (lower.endsWith(QStringLiteral(".appimage"))) return 0;
        if (lower.endsWith(QStringLiteral(".deb"))) return 1;
        if (lower.endsWith(QStringLiteral(".tar.gz")) || lower.endsWith(QStringLiteral(".tgz"))) return 2;
    }
    return -1;
}

bool isSecureGitHubUrl(const QUrl &url, const QString &host)
{
    return url.isValid() && url.scheme() == QStringLiteral("https")
        && url.host().compare(host, Qt::CaseInsensitive) == 0
        && url.userInfo().isEmpty();
}
}

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
{
}

QUrl UpdateChecker::defaultEndpoint()
{
    return QUrl(QStringLiteral("https://api.github.com/repos/sharkwedy/holyscreen/releases?per_page=20"));
}

UpdateRelease UpdateChecker::parseGitHubRelease(
    const QByteArray &payload,
    const QString &currentVersion,
    const QString &platform)
{
    UpdateRelease result;
    if (payload.isEmpty() || payload.size() > maximumManifestSize) {
        result.error = QStringLiteral("Resposta de atualização vazia ou maior que 256 KiB.");
        return result;
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError
        || (!document.isObject() && !document.isArray())) {
        result.error = QStringLiteral("Resposta inválida da API de Releases do GitHub.");
        return result;
    }

    QJsonObject object;
    if (document.isArray()) {
        SemanticVersion newest;
        for (const auto &value : document.array()) {
            if (!value.isObject()) continue;
            const auto candidateObject = value.toObject();
            if (candidateObject.value(QStringLiteral("draft")).toBool()) continue;
            const auto candidate = parseVersion(
                candidateObject.value(QStringLiteral("tag_name")).toString());
            if (candidate.valid && (!newest.valid || newerThan(candidate, newest))) {
                newest = candidate;
                object = candidateObject;
            }
        }
        if (object.isEmpty()) {
            result.error = QStringLiteral("O GitHub não retornou uma release publicada compatível.");
            return result;
        }
    } else {
        object = document.object();
    }
    if (object.value(QStringLiteral("draft")).toBool()) {
        result.error = QStringLiteral("A API retornou uma release ainda em rascunho.");
        return result;
    }

    const auto tag = object.value(QStringLiteral("tag_name")).toString().trimmed();
    const auto latest = parseVersion(tag);
    const auto current = parseVersion(currentVersion);
    if (!latest.valid || !current.valid) {
        result.error = QStringLiteral("A release ou a versão instalada não usa SemVer compatível.");
        return result;
    }

    result.latestVersion = tag.startsWith(QLatin1Char('v')) ? tag.sliced(1) : tag;
    result.releaseUrl = QUrl(object.value(QStringLiteral("html_url")).toString());
    if (!isSecureGitHubUrl(result.releaseUrl, QStringLiteral("github.com"))) {
        result.error = QStringLiteral("A página da release não pertence ao GitHub via HTTPS.");
        return result;
    }
    result.available = newerThan(latest, current);

    static const QRegularExpression digestExpression(
        QStringLiteral(R"(^sha256:([0-9a-fA-F]{64})$)"));
    int selectedPriority = 100;
    for (const auto &value : object.value(QStringLiteral("assets")).toArray()) {
        if (!value.isObject()) continue;
        const auto asset = value.toObject();
        const auto name = asset.value(QStringLiteral("name")).toString();
        const auto priority = assetPriority(name, platform);
        if (priority < 0 || priority >= selectedPriority) continue;

        const auto digestMatch = digestExpression.match(asset.value(QStringLiteral("digest")).toString());
        const auto size = asset.value(QStringLiteral("size")).toVariant().toLongLong();
        const QUrl url(asset.value(QStringLiteral("browser_download_url")).toString());
        if (!digestMatch.hasMatch() || size <= 0 || size > maximumAssetSize
            || !isSecureGitHubUrl(url, QStringLiteral("github.com"))) {
            continue;
        }

        selectedPriority = priority;
        result.assetName = name;
        result.downloadUrl = url;
        result.sha256 = digestMatch.captured(1).toLower();
        result.assetSize = size;
    }
    return result;
}

void UpdateChecker::check(const QUrl &url, const QString &currentVersion)
{
    const auto endpoint = url.isEmpty() ? defaultEndpoint() : url;
    const auto trustedPath = endpoint.path() == QStringLiteral("/repos/sharkwedy/holyscreen/releases")
        || endpoint.path() == QStringLiteral("/repos/sharkwedy/holyscreen/releases/latest");
    if (!isSecureGitHubUrl(endpoint, QStringLiteral("api.github.com")) || !trustedPath) {
        emit completed({}, {}, false, QStringLiteral("Endpoint de atualização do GitHub inválido."));
        return;
    }

    QNetworkRequest request(endpoint);
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    request.setRawHeader("User-Agent", QByteArray("HolyScreen/") + currentVersion.toUtf8());
    request.setTransferTimeout(10'000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    auto *reply = m_network.get(request);
    auto payload = std::make_shared<QByteArray>();
    auto tooLarge = std::make_shared<bool>(false);
    connect(reply, &QIODevice::readyRead, this, [reply, payload, tooLarge] {
        payload->append(reply->readAll());
        if (payload->size() > maximumManifestSize) {
            *tooLarge = true;
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, payload, tooLarge, currentVersion] {
        payload->append(reply->readAll());
        if (*tooLarge || payload->size() > maximumManifestSize) {
            reply->deleteLater();
            emit completed({}, {}, false, QStringLiteral("Resposta de atualização maior que 256 KiB."));
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            const auto error = reply->errorString();
            reply->deleteLater();
            emit completed({}, {}, false, error);
            return;
        }
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200) {
            const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            reply->deleteLater();
            emit completed({}, {}, false,
                           QStringLiteral("GitHub respondeu com HTTP %1.").arg(status));
            return;
        }

        const auto release = parseGitHubRelease(*payload, currentVersion, runtimePlatform());

        reply->deleteLater();
        emit completed(release.latestVersion, release.releaseUrl,
                       release.available, release.error);
    });
}

} // namespace churchpresenter
