#pragma once

#include "bible/BibleImportTypes.h"

#include <QString>
#include <QVector>

#include <optional>

namespace churchpresenter {

class BibleRepository final {
public:
    explicit BibleRepository(QString databasePath);
    ~BibleRepository();

    BibleRepository(const BibleRepository &) = delete;
    BibleRepository &operator=(const BibleRepository &) = delete;

    bool open();
    [[nodiscard]] QString saveTranslation(BibleTranslation translation);
    [[nodiscard]] QVector<BibleTranslation> translations() const;
    bool importVerses(const QString &translationId, const QVector<BibleVerse> &verses);
    bool replaceImportedTranslation(
        const PlannedBibleTranslation &translation,
        const BibleImportCancellation &cancel = {});
    [[nodiscard]] std::optional<BibleTranslationSource> translationSource(
        const QString &translationId) const;
    [[nodiscard]] QVector<BibleVerse> verses(
        const QString &translationId, const BibleReference &reference) const;
    [[nodiscard]] QVector<int> chapters(
        const QString &translationId, BibleBook book) const;
    [[nodiscard]] QVector<int> verseNumbers(
        const QString &translationId, BibleBook book, int chapter) const;
    [[nodiscard]] QVector<BibleVerse> search(
        const QString &translationId, const QString &text, int limit = 100) const;

private:
    QString m_databasePath;
    QString m_connectionName;
};

} // namespace churchpresenter
