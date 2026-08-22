#pragma once

#include "bible/BibleTypes.h"

#include <QByteArray>
#include <QString>

namespace churchpresenter {

struct BibleJsonImportResult {
    BibleTranslation translation;
    QVector<BibleVerse> verses;
    QString error;

    [[nodiscard]] bool isValid() const { return error.isEmpty() && !verses.isEmpty(); }
};

class BibleJsonImporter final {
public:
    [[nodiscard]] BibleJsonImportResult parse(const QByteArray &json) const;
};

} // namespace churchpresenter
