#include "app/UpdateDownloader.h"

#include "app/UpdateChecker.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>

namespace churchpresenter {

struct UpdateDownloader::Transfer {
    QSaveFile file;
    QCryptographicHash hash{QCryptographicHash::Sha256};
    QString expectedSha256;
    qint64 expectedSize = 0;
    qint64 received = 0;
    bool aborted = false;

    explicit Transfer(const QString &path) : file(path) {}
};

UpdateDownloader::UpdateDownloader(QObject *parent)
    : UpdateDownloader(&UpdateChecker::isTrustedDownloadUrl, parent)
{
}

UpdateDownloader::UpdateDownloader(UrlValidator validator, QObject *parent)
    : QObject(parent)
    , m_validator(std::move(validator))
{
}

UpdateDownloader::~UpdateDownloader() = default;

bool UpdateDownloader::isRunning() const { return m_transfer != nullptr; }

void UpdateDownloader::fail(const QString &error)
{
    if (m_transfer) {
        m_transfer->file.cancelWriting();
        m_transfer.reset();
    }
    if (m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    emit finished({}, error);
}

void UpdateDownloader::start(const QUrl &url, const QString &expectedSha256, qint64 expectedSize,
                             const QString &destinationDirectory, const QString &fileName)
{
    if (isRunning()) {
        emit finished({}, tr("Já existe um download de atualização em andamento."));
        return;
    }
    if (!m_validator || !m_validator(url)) {
        emit finished({}, tr("O endereço do pacote não é uma origem confiável."));
        return;
    }
    static const QRegularExpression digestExpression(
        QStringLiteral(R"(^[0-9a-fA-F]{64}$)"));
    if (expectedSize <= 0 || !digestExpression.match(expectedSha256).hasMatch()) {
        emit finished({}, tr("A release não publicou tamanho e digest utilizáveis."));
        return;
    }
    // O nome vem da release; usar apenas o componente final impede que um
    // caminho relativo escreva fora do diretório de destino.
    const auto safeName = QFileInfo(fileName).fileName();
    if (safeName.isEmpty()) {
        emit finished({}, tr("A release não publicou um nome de arquivo utilizável."));
        return;
    }
    if (!QDir().mkpath(destinationDirectory)) {
        emit finished({}, tr("Não foi possível criar a pasta de downloads."));
        return;
    }

    const auto path = QDir(destinationDirectory).filePath(safeName);
    m_transfer = std::make_unique<Transfer>(path);
    m_transfer->expectedSha256 = expectedSha256.toLower();
    m_transfer->expectedSize = expectedSize;
    if (!m_transfer->file.open(QIODevice::WriteOnly)) {
        const auto reason = m_transfer->file.errorString();
        m_transfer.reset();
        emit finished({}, tr("Não foi possível gravar em %1: %2").arg(path, reason));
        return;
    }

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", QByteArrayLiteral("HolyScreen"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    m_reply = m_network.get(request);

    connect(m_reply, &QIODevice::readyRead, this, [this] {
        if (!m_transfer || !m_reply) return;
        const auto chunk = m_reply->readAll();
        m_transfer->received += chunk.size();
        if (m_transfer->received > m_transfer->expectedSize) {
            // Um corpo maior que o anunciado não corresponde à release.
            m_transfer->aborted = true;
            m_reply->abort();
            return;
        }
        m_transfer->hash.addData(chunk);
        if (m_transfer->file.write(chunk) != chunk.size()) {
            m_transfer->aborted = true;
            m_reply->abort();
        }
    });
    connect(m_reply, &QNetworkReply::downloadProgress, this,
            [this](qint64 received, qint64 total) { emit progress(received, total); });
    connect(m_reply, &QNetworkReply::finished, this, [this] {
        if (!m_transfer || !m_reply) return;
        const auto aborted = m_transfer->aborted;
        const auto networkError = m_reply->error();
        const auto errorString = m_reply->errorString();
        const auto status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (aborted) {
            fail(tr("O pacote recebido não corresponde ao tamanho anunciado."));
            return;
        }
        if (networkError == QNetworkReply::OperationCanceledError) {
            fail(tr("Download cancelado."));
            return;
        }
        if (networkError != QNetworkReply::NoError) {
            fail(errorString);
            return;
        }
        if (status != 0 && status != 200) {
            fail(tr("O servidor respondeu com HTTP %1.").arg(status));
            return;
        }
        if (m_transfer->received != m_transfer->expectedSize) {
            fail(tr("O pacote recebido tem %1 byte(s), e a release anuncia %2.")
                     .arg(m_transfer->received).arg(m_transfer->expectedSize));
            return;
        }

        const auto digest = QString::fromLatin1(m_transfer->hash.result().toHex());
        if (digest != m_transfer->expectedSha256) {
            fail(tr("O SHA-256 do pacote não confere com o publicado na release."));
            return;
        }
        if (!m_transfer->file.commit()) {
            const auto reason = m_transfer->file.errorString();
            fail(tr("Não foi possível concluir a gravação: %1").arg(reason));
            return;
        }

        const auto path = m_transfer->file.fileName();
        m_transfer.reset();
        m_reply->deleteLater();
        m_reply = nullptr;
        emit finished(path, {});
    });
}

void UpdateDownloader::cancel()
{
    if (m_reply) m_reply->abort();
}

} // namespace churchpresenter
