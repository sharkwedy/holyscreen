#include "bible/BibleSourceStager.h"
#include "bible/PathContainment.h"

#include <QDir>
#include <QCryptographicHash>
#include <QEventLoop>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QTimer>

#include <git2.h>
#include <miniz.h>

#include <limits>

namespace churchpresenter {

namespace {

constexpr qint64 maximumArchiveBytes = 512LL * 1024 * 1024;
constexpr size_t maximumGitBytes = 1024ULL * 1024 * 1024;
constexpr mz_uint64 maximumExtractedBytes = 2ULL * 1024 * 1024 * 1024;
constexpr mz_uint maximumArchiveEntries = 10000;

void report(const BibleImportProgressCallback &callback, BibleImportPhase phase,
            int current, int total, const QString &message)
{
    if (callback) callback({phase, current, total, {}, message});
}

QString gitError(const QString &fallback)
{
    const auto *error = git_error_last();
    return error && error->message
        ? QString::fromUtf8(error->message)
        : fallback;
}

struct CloneProgressContext {
    BibleImportProgressCallback progress;
    BibleImportCancellation cancel;
    bool exceededSizeLimit = false;
};

int onGitTransferProgress(const git_indexer_progress *stats, void *payload)
{
    auto *context = static_cast<CloneProgressContext *>(payload);
    if (context->cancel && context->cancel()) return -1;
    if (stats->received_bytes > maximumGitBytes) {
        context->exceededSizeLimit = true;
        return -1;
    }
    if (context->progress) {
        context->progress({BibleImportPhase::Cloning,
                           static_cast<int>(stats->received_objects),
                           static_cast<int>(stats->total_objects), {},
                           QStringLiteral("Baixando objetos do repositório...")});
    }
    return 0;
}

bool isAllowedUrl(const QUrl &url, bool allowLocal)
{
    if (!url.isValid() || url.isRelative()) return false;
    if (url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0) {
        return !url.host().isEmpty() && url.userInfo().isEmpty();
    }
    return allowLocal && url.isLocalFile();
}

} // namespace

bool ZipArchiveExtractor::isSafeEntryName(const QString &entryName)
{
    auto normalized = entryName;
    normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (normalized.isEmpty() || normalized.startsWith(QLatin1Char('/'))
        || normalized.contains(QChar::Null)
        || (normalized.size() >= 2 && normalized.at(1) == QLatin1Char(':'))) return false;
    if (normalized.split(QLatin1Char('/')).contains(QStringLiteral(".."))) return false;
    const auto clean = QDir::cleanPath(normalized);
    return clean != QStringLiteral("..")
        && !clean.startsWith(QStringLiteral("../"))
        && !clean.contains(QStringLiteral("/../"));
}

bool ZipArchiveExtractor::extract(const QString &archivePath, const QString &destination,
                                  QString *error,
                                  const BibleImportProgressCallback &progress,
                                  const BibleImportCancellation &cancel) const
{
    if (!QDir().mkpath(destination)) {
        if (error) *error = QStringLiteral("Não foi possível criar a pasta de staging.");
        return false;
    }
    const auto destinationRoot = QFileInfo(destination).canonicalFilePath();
    mz_zip_archive archive{};
    const auto archiveBytes = QFile::encodeName(archivePath);
    if (!mz_zip_reader_init_file(&archive, archiveBytes.constData(), 0)) {
        if (error) *error = QStringLiteral("O arquivo ZIP é inválido ou não pôde ser aberto.");
        return false;
    }
    const auto closeArchive = [&archive] { mz_zip_reader_end(&archive); };
    const auto entryCount = mz_zip_reader_get_num_files(&archive);
    if (entryCount == 0 || entryCount > maximumArchiveEntries) {
        closeArchive();
        if (error) *error = QStringLiteral("O ZIP está vazio ou excede o limite de arquivos.");
        return false;
    }

    mz_uint64 extractedBytes = 0;
    for (mz_uint index = 0; index < entryCount; ++index) {
        if (cancel && cancel()) {
            closeArchive();
            if (error) *error = QStringLiteral("Importação cancelada.");
            return false;
        }
        mz_zip_archive_file_stat stat{};
        if (!mz_zip_reader_file_stat(&archive, index, &stat)) {
            closeArchive();
            if (error) *error = QStringLiteral("Não foi possível ler uma entrada do ZIP.");
            return false;
        }
        const auto entryName = QString::fromUtf8(stat.m_filename);
        if (!isSafeEntryName(entryName)) {
            closeArchive();
            if (error) *error = QStringLiteral("O ZIP contém um caminho inseguro: %1").arg(entryName);
            return false;
        }
        const auto unixMode = (stat.m_external_attr >> 16U) & 0170000U;
        if (unixMode == 0120000U) {
            closeArchive();
            if (error) *error = QStringLiteral("Links simbólicos não são aceitos no ZIP.");
            return false;
        }
        if (stat.m_uncomp_size > maximumExtractedBytes - extractedBytes) {
            closeArchive();
            if (error) *error = QStringLiteral("O conteúdo extraído excede o limite de 2 GB.");
            return false;
        }
        extractedBytes += stat.m_uncomp_size;
        auto normalized = entryName;
        normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
        const auto outputPath = QDir(destinationRoot).absoluteFilePath(QDir::cleanPath(normalized));
        if (!isPathContained(destinationRoot, outputPath)) {
            closeArchive();
            if (error) *error = QStringLiteral("O ZIP tentou escrever fora do staging.");
            return false;
        }
        if (mz_zip_reader_is_file_a_directory(&archive, index)) {
            if (!QDir().mkpath(outputPath)) {
                closeArchive();
                if (error) *error = QStringLiteral("Não foi possível criar uma pasta do ZIP.");
                return false;
            }
        } else {
            if (!QDir().mkpath(QFileInfo(outputPath).absolutePath())) {
                closeArchive();
                if (error) *error = QStringLiteral("Não foi possível criar a pasta de um arquivo do ZIP.");
                return false;
            }
            const auto outputBytes = QFile::encodeName(outputPath);
            if (!mz_zip_reader_extract_to_file(&archive, index, outputBytes.constData(), 0)) {
                closeArchive();
                if (error) *error = QStringLiteral("Falha ao extrair %1.").arg(entryName);
                return false;
            }
        }
        report(progress, BibleImportPhase::Extracting, static_cast<int>(index + 1),
               static_cast<int>(entryCount), QStringLiteral("Extraindo %1").arg(entryName));
    }
    closeArchive();
    return true;
}

GitBibleSourceStager::GitBibleSourceStager(bool allowLocalRepositories)
    : m_allowLocalRepositories(allowLocalRepositories)
{
}

StagedBibleSource GitBibleSourceStager::stage(
    const BibleSource &source, const QString &destination,
    const BibleImportProgressCallback &progress,
    const BibleImportCancellation &cancel) const
{
    StagedBibleSource result{.source = source};
    const QUrl url(source.location, QUrl::StrictMode);
    if (!isAllowedUrl(url, m_allowLocalRepositories)) {
        result.error = QStringLiteral("Informe uma URL Git HTTPS pública e sem credenciais.");
        return result;
    }
    if (cancel && cancel()) {
        result.cancelled = true;
        result.error = QStringLiteral("Importação cancelada.");
        return result;
    }
    if (!QDir().mkpath(QFileInfo(destination).absolutePath())) {
        result.error = QStringLiteral("Não foi possível criar o staging do Git.");
        return result;
    }

    git_libgit2_init();
    git_repository *repository = nullptr;
    git_clone_options options = GIT_CLONE_OPTIONS_INIT;
    options.checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
    options.fetch_opts.download_tags = GIT_REMOTE_DOWNLOAD_TAGS_NONE;
    CloneProgressContext context{progress, cancel, false};
    options.fetch_opts.callbacks.transfer_progress = onGitTransferProgress;
    options.fetch_opts.callbacks.payload = &context;
    const auto urlBytes = url.isLocalFile() ? QFile::encodeName(url.toLocalFile()) : url.toEncoded();
    const auto destinationBytes = QFile::encodeName(destination);
    report(progress, BibleImportPhase::Cloning, 0, 0,
           QStringLiteral("Clonando repositório público..."));
    const int cloneStatus = git_clone(&repository, urlBytes.constData(),
                                      destinationBytes.constData(), &options);
    if (cloneStatus != 0) {
        result.cancelled = cancel && cancel();
        result.error = result.cancelled ? QStringLiteral("Importação cancelada.")
            : context.exceededSizeLimit ? QStringLiteral("O repositório Git excede o limite de 1 GB.")
                                        : gitError(QStringLiteral("Falha ao clonar o repositório."));
        if (repository) git_repository_free(repository);
        git_libgit2_shutdown();
        return result;
    }

    git_reference *head = nullptr;
    git_object *commit = nullptr;
    if (git_repository_head(&head, repository) != 0
        || git_reference_peel(&commit, head, GIT_OBJECT_COMMIT) != 0) {
        result.error = gitError(QStringLiteral("Não foi possível resolver a revisão Git."));
    } else {
        char oid[GIT_OID_SHA1_HEXSIZE + 1]{};
        git_oid_tostr(oid, sizeof(oid), git_object_id(commit));
        result.source.revision = QString::fromLatin1(oid);
        result.localPath = destination;
        result.success = true;
    }
    if (commit) git_object_free(commit);
    if (head) git_reference_free(head);
    git_repository_free(repository);
    git_libgit2_shutdown();
    return result;
}

ZipBibleSourceStager::ZipBibleSourceStager(bool allowLocalFiles)
    : m_allowLocalFiles(allowLocalFiles)
{
}

StagedBibleSource ZipBibleSourceStager::stage(
    const BibleSource &source, const QString &destination,
    const BibleImportProgressCallback &progress,
    const BibleImportCancellation &cancel) const
{
    StagedBibleSource result{.source = source};
    const QUrl url(source.location, QUrl::StrictMode);
    if (!isAllowedUrl(url, m_allowLocalFiles)) {
        result.error = QStringLiteral("Informe uma URL ZIP HTTPS pública e sem credenciais.");
        return result;
    }
    if (!QDir().mkpath(destination)) {
        result.error = QStringLiteral("Não foi possível criar o staging do ZIP.");
        return result;
    }

    const auto archivePath = QDir(destination).filePath(QStringLiteral("source.zip"));
    QSaveFile archiveFile(archivePath);
    if (!archiveFile.open(QIODevice::WriteOnly)) {
        result.error = QStringLiteral("Não foi possível criar o arquivo temporário do ZIP.");
        return result;
    }
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setMaximumRedirectsAllowed(5);
    request.setTransferTimeout(60000);
    auto *reply = manager.get(request);
    QEventLoop loop;
    QTimer cancellationTimer;
    cancellationTimer.setInterval(50);
    qint64 received = 0;
    bool tooLarge = false;
    QCryptographicHash archiveHash(QCryptographicHash::Sha256);
    QObject::connect(reply, &QNetworkReply::readyRead, &loop, [&] {
        const auto chunk = reply->readAll();
        received += chunk.size();
        if (received > maximumArchiveBytes || archiveFile.write(chunk) != chunk.size()) {
            tooLarge = true;
            reply->abort();
            return;
        }
        archiveHash.addData(chunk);
        report(progress, BibleImportPhase::Downloading,
               static_cast<int>(std::min<qint64>(received, std::numeric_limits<int>::max())),
               static_cast<int>(std::min<qint64>(reply->header(QNetworkRequest::ContentLengthHeader).toLongLong(),
                                                  std::numeric_limits<int>::max())),
               QStringLiteral("Baixando arquivo ZIP..."));
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&cancellationTimer, &QTimer::timeout, reply, [&] {
        if (cancel && cancel()) reply->abort();
    });
    cancellationTimer.start();
    loop.exec();
    cancellationTimer.stop();
    const bool cancelled = cancel && cancel();
    const auto networkError = reply->error();
    const auto networkErrorText = reply->errorString();
    reply->deleteLater();
    if (cancelled || tooLarge || networkError != QNetworkReply::NoError) {
        archiveFile.cancelWriting();
        result.cancelled = cancelled;
        result.error = cancelled ? QStringLiteral("Importação cancelada.")
            : tooLarge ? QStringLiteral("O ZIP excede o limite de 512 MB.")
                       : QStringLiteral("Falha ao baixar o ZIP: %1").arg(networkErrorText);
        return result;
    }
    if (!archiveFile.commit()) {
        result.error = QStringLiteral("Não foi possível finalizar o ZIP temporário.");
        return result;
    }
    result.source.revision = QString::fromLatin1(archiveHash.result().toHex());

    const auto extractedPath = QDir(destination).filePath(QStringLiteral("extracted"));
    if (!ZipArchiveExtractor{}.extract(archivePath, extractedPath, &result.error,
                                       progress, cancel)) {
        result.cancelled = cancel && cancel();
        return result;
    }
    result.localPath = extractedPath;
    result.success = true;
    return result;
}

} // namespace churchpresenter
