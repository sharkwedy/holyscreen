#include "bible/CanonicalBibleImporter.h"
#include "bible/PathContainment.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QDirIterator>

namespace churchpresenter {

namespace {

std::optional<QJsonObject> readObject(const QString &path, QString *error,
                                      QCryptographicHash *hash = nullptr)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("Não foi possível abrir %1.").arg(path);
        return std::nullopt;
    }
    const auto contents = file.readAll();
    if (hash) hash->addData(contents);
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(contents, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) {
            *error = QStringLiteral("JSON inválido em %1: %2")
                         .arg(path, parseError.errorString());
        }
        return std::nullopt;
    }
    return document.object();
}

QStringList translationDirectories(const QString &sourcePath, QString *error)
{
    QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists() || !sourceInfo.isDir() || sourceInfo.isSymLink()) {
        if (error) *error = QStringLiteral("A pasta de origem não existe ou não é segura.");
        return {};
    }

    const auto sourceCanonical = sourceInfo.canonicalFilePath();
    QString rootPath;
    if (QFileInfo(QDir(sourceCanonical).filePath(QStringLiteral("meta.json"))).isFile()) {
        return {sourceCanonical};
    }
    const auto nestedCanonical = QDir(sourceCanonical).filePath(QStringLiteral("data/canonical"));
    if (QFileInfo(nestedCanonical).isDir()) {
        rootPath = QFileInfo(nestedCanonical).canonicalFilePath();
    } else {
        QDirIterator iterator(sourceCanonical, QDir::Dirs | QDir::NoDotAndDotDot,
                              QDirIterator::Subdirectories);
        QStringList candidates;
        int visited = 0;
        while (iterator.hasNext() && ++visited <= 10000) {
            iterator.next();
            const auto info = iterator.fileInfo();
            if (info.isSymLink() || info.fileName() != QStringLiteral("canonical")) continue;
            const QFileInfo parent(info.absolutePath());
            if (parent.fileName() != QStringLiteral("data")) continue;
            const auto candidate = info.canonicalFilePath();
            if (isPathContained(sourceCanonical, candidate)) candidates.append(candidate);
        }
        candidates.removeDuplicates();
        if (candidates.size() > 1) {
            if (error) *error = QStringLiteral("Mais de uma raiz data/canonical foi encontrada.");
            return {};
        }
        rootPath = candidates.isEmpty() ? sourceCanonical : candidates.front();
    }

    if (!isPathContained(sourceCanonical, rootPath)) {
        if (error) *error = QStringLiteral("A raiz canônica sai da pasta selecionada.");
        return {};
    }

    QDir root(rootPath);
    const auto entries = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
                                            QDir::Name | QDir::IgnoreCase);
    QStringList result;
    for (const auto &entry : entries) {
        if (entry.isSymLink()) continue;
        const auto canonical = entry.canonicalFilePath();
        if (!isPathContained(rootPath, canonical)) continue;
        if (QFileInfo(QDir(canonical).filePath(QStringLiteral("meta.json"))).isFile()) {
            result.append(canonical);
        }
    }
    if (result.isEmpty() && error) {
        *error = QStringLiteral("Nenhuma tradução foi encontrada em data/canonical.");
    }
    return result;
}

std::optional<PlannedBibleTranslation> parseTranslation(
    const QString &directoryPath, const BibleSource &source, QString *error,
    const BibleImportCancellation &cancel)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    const auto metaPath = QDir(directoryPath).filePath(QStringLiteral("meta.json"));
    const auto meta = readObject(metaPath, error, &hash);
    if (!meta.has_value()) return std::nullopt;

    const auto code = meta->value(QStringLiteral("code")).toString().trimmed().toUpper();
    const auto name = meta->value(QStringLiteral("name")).toString().trimmed();
    const auto license = meta->value(QStringLiteral("license")).toString().trimmed().toLower();
    auto language = meta->value(QStringLiteral("language")).toString().trimmed();
    if (language.isEmpty()) language = QStringLiteral("pt-BR");
    if (code.isEmpty() || name.isEmpty() || license.isEmpty()) {
        if (error) *error = QStringLiteral("Metadados incompletos em %1.").arg(metaPath);
        return std::nullopt;
    }

    PlannedBibleTranslation planned;
    planned.translation = {
        .id = QStringLiteral("canonical:%1").arg(code),
        .name = name,
        .abbreviation = code,
        .language = language,
    };
    planned.source = {
        .translationId = planned.translation.id,
        .kind = source.kind,
        .location = source.location,
        .revision = source.revision,
        .license = license,
        .publisher = meta->value(QStringLiteral("publisher")).toString().trimmed(),
        .sourceName = meta->value(QStringLiteral("source")).toString().trimmed(),
        .sourceCode = code,
        .scope = meta->value(QStringLiteral("scope")).toString().trimmed(),
    };
    planned.requiresLicenseConfirmation = license != QStringLiteral("public-domain");

    QDir directory(directoryPath);
    const auto files = directory.entryInfoList({QStringLiteral("*.json")}, QDir::Files,
                                               QDir::Name | QDir::IgnoreCase);
    QSet<QString> references;
    for (const auto &file : files) {
        if (cancel && cancel()) {
            if (error) *error = QStringLiteral("Importação cancelada.");
            return std::nullopt;
        }
        if (file.fileName().compare(QStringLiteral("meta.json"), Qt::CaseInsensitive) == 0) continue;
        if (file.isSymLink() || !isPathContained(directoryPath, file.canonicalFilePath())) {
            if (error) *error = QStringLiteral("Arquivo inseguro: %1.").arg(file.filePath());
            return std::nullopt;
        }
        hash.addData(file.fileName().toUtf8());
        const auto bookObject = readObject(file.filePath(), error, &hash);
        if (!bookObject.has_value()) return std::nullopt;
        const auto bookId = bookObject->value(QStringLiteral("id")).toInt();
        if (bookId < static_cast<int>(BibleBook::Genesis)
            || bookId > static_cast<int>(BibleBook::Revelation)) {
            if (error) *error = QStringLiteral("Livro inválido em %1.").arg(file.filePath());
            return std::nullopt;
        }
        const auto book = static_cast<BibleBook>(bookId);
        const auto chapters = bookObject->value(QStringLiteral("chapters")).toArray();
        if (chapters.isEmpty()) {
            if (error) *error = QStringLiteral("Livro sem capítulos em %1.").arg(file.filePath());
            return std::nullopt;
        }
        for (const auto &chapterValue : chapters) {
            const auto chapterObject = chapterValue.toObject();
            const auto chapterNumber = chapterObject.value(QStringLiteral("number")).toInt();
            const auto verses = chapterObject.value(QStringLiteral("verses")).toArray();
            if (chapterNumber <= 0 || verses.isEmpty()) {
                if (error) *error = QStringLiteral("Capítulo inválido em %1.").arg(file.filePath());
                return std::nullopt;
            }
            for (const auto &verseValue : verses) {
                if (cancel && cancel()) {
                    if (error) *error = QStringLiteral("Importação cancelada.");
                    return std::nullopt;
                }
                const auto verseObject = verseValue.toObject();
                const auto verseNumber = verseObject.value(QStringLiteral("number")).toInt();
                const auto text = verseObject.value(QStringLiteral("text")).toString().trimmed();
                const auto key = QStringLiteral("%1:%2:%3").arg(bookId).arg(chapterNumber).arg(verseNumber);
                if (verseNumber <= 0 || text.isEmpty() || references.contains(key)) {
                    if (error) *error = QStringLiteral("Versículo inválido ou duplicado em %1.").arg(file.filePath());
                    return std::nullopt;
                }
                references.insert(key);
                planned.verses.append({planned.translation.id, book, chapterNumber, verseNumber, text});
            }
        }
    }
    if (planned.verses.isEmpty()) {
        if (error) *error = QStringLiteral("A tradução %1 não contém livros válidos.").arg(code);
        return std::nullopt;
    }
    planned.source.contentHash = QString::fromLatin1(hash.result().toHex());
    return planned;
}

} // namespace

BibleImportPlan CanonicalBibleImporter::inspect(const BibleSource &source) const
{
    return inspect(source, source.location, {}, {});
}

BibleImportPlan CanonicalBibleImporter::inspect(
    const BibleSource &source, const QString &localPath,
    const BibleImportProgressCallback &progress,
    const BibleImportCancellation &cancel) const
{
    BibleImportPlan plan;
    QString discoveryError;
    const auto directories = translationDirectories(localPath, &discoveryError);
    if (directories.isEmpty()) {
        plan.errors.append(discoveryError);
        return plan;
    }
    QSet<QString> translationIds;
    for (int index = 0; index < directories.size(); ++index) {
        if (cancel && cancel()) {
            plan.errors.append(QStringLiteral("Importação cancelada."));
            return plan;
        }
        const auto &directory = directories.at(index);
        QString error;
        auto translation = parseTranslation(directory, source, &error, cancel);
        if (!translation.has_value()) {
            plan.errors.append(error);
            continue;
        }
        if (translationIds.contains(translation->translation.id)) {
            plan.errors.append(QStringLiteral("Código de tradução duplicado: %1")
                                   .arg(translation->translation.abbreviation));
            continue;
        }
        translationIds.insert(translation->translation.id);
        plan.translations.append(std::move(*translation));
        if (progress) {
            progress({BibleImportPhase::Inspecting, index + 1,
                      static_cast<int>(directories.size()),
                      plan.translations.back().translation.abbreviation,
                      QStringLiteral("Validando tradução %1...")
                          .arg(plan.translations.back().translation.abbreviation)});
        }
    }
    return plan;
}

} // namespace churchpresenter
