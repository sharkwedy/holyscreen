#include "bible/BibleImportService.h"

#include "bible/BibleRepository.h"
#include "bible/BibleSourceStager.h"
#include "bible/CanonicalBibleImporter.h"

#include <QTemporaryDir>

namespace churchpresenter {

BibleImportService::BibleImportService(QString databasePath)
    : m_databasePath(std::move(databasePath))
{
}

BibleImportResult BibleImportService::run(
    const BibleSource &source, const BibleImportOptions &options,
    const BibleImportProgressCallback &progress,
    const BibleImportCancellation &cancel) const
{
    BibleImportResult result;
    if (cancel && cancel()) {
        result.cancelled = true;
        return result;
    }

    QTemporaryDir staging;
    QString localPath = source.location;
    BibleSource resolvedSource = source;
    if (source.kind == BibleSourceKind::GitHttps) {
        if (!staging.isValid()) {
            result.errors.append(QStringLiteral("Não foi possível criar o staging temporário."));
            return result;
        }
        const auto staged = GitBibleSourceStager{}.stage(
            source, staging.filePath(QStringLiteral("repository")), progress, cancel);
        if (!staged.success) {
            result.cancelled = staged.cancelled;
            result.errors.append(staged.error);
            return result;
        }
        localPath = staged.localPath;
        resolvedSource = staged.source;
    } else if (source.kind == BibleSourceKind::ZipUrl) {
        if (!staging.isValid()) {
            result.errors.append(QStringLiteral("Não foi possível criar o staging temporário."));
            return result;
        }
        const auto staged = ZipBibleSourceStager{}.stage(
            source, staging.filePath(QStringLiteral("zip")), progress, cancel);
        if (!staged.success) {
            result.cancelled = staged.cancelled;
            result.errors.append(staged.error);
            return result;
        }
        localPath = staged.localPath;
        resolvedSource = staged.source;
    } else if (source.kind != BibleSourceKind::LocalFolder) {
        result.errors.append(QStringLiteral("Tipo de origem ainda não suportado por este importador."));
        return result;
    }

    if (progress) {
        progress({BibleImportPhase::Inspecting, 0, 0, {},
                  QStringLiteral("Detectando traduções e livros...")});
    }
    const auto plan = CanonicalBibleImporter{}.inspect(
        resolvedSource, localPath, progress, cancel);
    if (!plan.isValid()) {
        result.cancelled = cancel && cancel();
        result.errors = plan.errors;
        return result;
    }
    result.errors = plan.errors;
    result.failedTranslations = plan.errors.size();

    for (const auto &translation : plan.translations) {
        if (translation.requiresLicenseConfirmation) {
            result.restrictedTranslations.append(
                QStringLiteral("%1 — %2")
                    .arg(translation.translation.abbreviation, translation.source.license));
        }
    }
    if (!result.restrictedTranslations.isEmpty() && !options.confirmRestrictedLicenses) {
        result.requiresLicenseConfirmation = true;
        return result;
    }

    BibleRepository repository(m_databasePath);
    if (!repository.open()) {
        result.errors.append(QStringLiteral("Não foi possível abrir o banco para importação."));
        return result;
    }
    for (int index = 0; index < plan.translations.size(); ++index) {
        if (cancel && cancel()) {
            result.cancelled = true;
            return result;
        }
        const auto &translation = plan.translations.at(index);
        if (progress) {
            progress({BibleImportPhase::Saving, index + 1,
                      static_cast<int>(plan.translations.size()),
                      translation.translation.abbreviation,
                      QStringLiteral("Salvando tradução %1...")
                          .arg(translation.translation.abbreviation)});
        }
        if (!repository.replaceImportedTranslation(translation, cancel)) {
            if (cancel && cancel()) {
                result.cancelled = true;
                return result;
            }
            result.errors.append(QStringLiteral("Falha ao salvar %1; essa tradução foi revertida.")
                                     .arg(translation.translation.abbreviation));
            ++result.failedTranslations;
            return result;
        }
        ++result.importedTranslations;
        result.importedVerses += translation.verses.size();
    }
    result.success = true;
    if (progress) {
        progress({BibleImportPhase::Completed, result.importedTranslations,
                  result.importedTranslations, {},
                  QStringLiteral("Importação concluída.")});
    }
    return result;
}

} // namespace churchpresenter
