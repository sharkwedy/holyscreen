#include "app/DiagnosticExporter.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>

#include <miniz.h>

namespace {

bool isSensitiveKey(const QString &key)
{
    const auto normalized = key.toLower();
    return normalized.contains(QStringLiteral("password"))
        || normalized.contains(QStringLiteral("passwd"))
        || normalized.contains(QStringLiteral("token"))
        || normalized.contains(QStringLiteral("secret"))
        || normalized.contains(QStringLiteral("credential"))
        || normalized.contains(QStringLiteral("apikey"))
        || normalized.contains(QStringLiteral("api_key"))
        || normalized.endsWith(QStringLiteral("hash"))
        || normalized.endsWith(QStringLiteral("salt"));
}

QVariant sanitizeValue(const QVariant &value);

QVariantMap sanitizeMap(const QVariantMap &map)
{
    QVariantMap result;
    for (auto iterator = map.cbegin(); iterator != map.cend(); ++iterator) {
        if (isSensitiveKey(iterator.key())) continue;
        result.insert(iterator.key(), sanitizeValue(iterator.value()));
    }
    return result;
}

QVariant sanitizeValue(const QVariant &value)
{
    if (value.metaType().id() == QMetaType::QVariantMap) {
        return sanitizeMap(value.toMap());
    }
    if (value.metaType().id() == QMetaType::QVariantList) {
        QVariantList sanitized;
        for (const auto &entry : value.toList()) sanitized.append(sanitizeValue(entry));
        return sanitized;
    }
    return value;
}

} // namespace

namespace churchpresenter {

QVariantMap DiagnosticExporter::sanitize(const QVariantMap &configuration)
{
    return sanitizeMap(configuration);
}

bool DiagnosticExporter::exportZip(const DiagnosticExportRequest &request, QString *error)
{
    const auto destination = QDir::cleanPath(request.destinationPath.trimmed());
    if (destination.isEmpty()) {
        if (error) *error = QStringLiteral("O destino do diagnóstico é obrigatório.");
        return false;
    }
    const QFileInfo destinationInfo(destination);
    if (!QDir().mkpath(destinationInfo.absolutePath())) {
        if (error) *error = QStringLiteral("Não foi possível criar a pasta de destino.");
        return false;
    }

    QVariantMap document{
        {QStringLiteral("generatedAt"),
         QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {QStringLiteral("application"), sanitize(request.application)},
        {QStringLiteral("screens"), sanitizeValue(request.screens)},
        {QStringLiteral("configuration"), sanitize(request.configuration)},
    };
    const auto json = QJsonDocument::fromVariant(document).toJson(QJsonDocument::Indented);

    mz_zip_archive archive{};
    const auto encodedDestination = QFile::encodeName(destination);
    if (!mz_zip_writer_init_file(&archive, encodedDestination.constData(), 0)) {
        if (error) *error = QStringLiteral("Não foi possível criar o arquivo ZIP.");
        return false;
    }
    bool succeeded = mz_zip_writer_add_mem(&archive, "diagnostics.json",
                                           json.constData(), static_cast<size_t>(json.size()),
                                           MZ_BEST_COMPRESSION);
    const QFileInfo logInfo(request.logPath);
    if (succeeded && logInfo.isFile() && logInfo.isReadable()) {
        const auto encodedLog = QFile::encodeName(logInfo.absoluteFilePath());
        succeeded = mz_zip_writer_add_file(&archive, "logs/holyscreen.log",
                                           encodedLog.constData(), nullptr, 0,
                                           MZ_BEST_COMPRESSION);
    }
    if (succeeded) succeeded = mz_zip_writer_finalize_archive(&archive);
    const auto archiveEnded = mz_zip_writer_end(&archive);
    succeeded = succeeded && archiveEnded;
    if (!succeeded) {
        if (error) *error = QStringLiteral("Não foi possível finalizar o diagnóstico ZIP.");
        return false;
    }
    if (error) error->clear();
    return true;
}

} // namespace churchpresenter
