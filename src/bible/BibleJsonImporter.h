#pragma once

#include "bible/BibleTypes.h"

#include <QByteArray>
#include <QString>

namespace churchpresenter {

struct BibleImportResult {
    BibleTranslation translation;
    QVector<BibleVerse> verses;
    QString error;

    [[nodiscard]] bool isValid() const { return error.isEmpty() && !verses.isEmpty(); }
};

class BibleJsonImporter final {
public:
    [[nodiscard]] BibleImportResult parse(const QByteArray &json) const;
};

} // namespace churchpresenter
