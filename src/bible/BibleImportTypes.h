#pragma once

#include "bible/BibleTypes.h"

#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>

namespace churchpresenter {

enum class BibleSourceKind {
    LocalFolder,
    GitHttps,
    ZipUrl,
    HolyScreenJson,
};

struct BibleSource {
    BibleSourceKind kind = BibleSourceKind::LocalFolder;
    QString location;
    QString revision;
};

struct BibleTranslationSource {
    QString translationId;
    BibleSourceKind kind = BibleSourceKind::LocalFolder;
    QString location;
    QString revision;
    QString license;
    QString publisher;
    QString sourceName;
    QString sourceCode;
    QString scope;
    QString importedAt;
    QString contentHash;

    bool operator==(const BibleTranslationSource &) const = default;
};

struct PlannedBibleTranslation {
    BibleTranslation translation;
    BibleTranslationSource source;
    QVector<BibleVerse> verses;
    bool requiresLicenseConfirmation = false;
};

struct BibleImportPlan {
    QVector<PlannedBibleTranslation> translations;
    QStringList errors;

    [[nodiscard]] bool isValid() const
    {
        return !translations.isEmpty();
    }
    [[nodiscard]] QString errorSummary() const { return errors.join(QLatin1Char('\n')); }
};

enum class BibleImportPhase {
    Staging,
    Downloading,
    Extracting,
    Cloning,
    Inspecting,
    Saving,
    Completed,
};

struct BibleImportProgress {
    BibleImportPhase phase = BibleImportPhase::Staging;
    int current = 0;
    int total = 0;
    QString translationCode;
    QString message;
};

struct BibleImportOptions {
    bool confirmRestrictedLicenses = false;
};

struct BibleImportResult {
    bool success = false;
    bool cancelled = false;
    bool requiresLicenseConfirmation = false;
    int importedTranslations = 0;
    int importedVerses = 0;
    int failedTranslations = 0;
    QStringList restrictedTranslations;
    QStringList errors;

    [[nodiscard]] QString errorSummary() const { return errors.join(QLatin1Char('\n')); }
};

using BibleImportProgressCallback = std::function<void(const BibleImportProgress &)>;
using BibleImportCancellation = std::function<bool()>;

[[nodiscard]] QString bibleSourceKindName(BibleSourceKind kind);

} // namespace churchpresenter
