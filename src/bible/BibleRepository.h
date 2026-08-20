#pragma once

#include "bible/BibleTypes.h"

#include <QString>
#include <QVector>

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
    [[nodiscard]] QVector<BibleVerse> verses(
        const QString &translationId, const BibleReference &reference) const;
    [[nodiscard]] QVector<BibleVerse> search(
        const QString &translationId, const QString &text, int limit = 100) const;

private:
    QString m_databasePath;
    QString m_connectionName;
};

} // namespace churchpresenter
