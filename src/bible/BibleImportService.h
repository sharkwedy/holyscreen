#pragma once

#include "bible/BibleImportTypes.h"

namespace churchpresenter {

class BibleImportService final {
public:
    explicit BibleImportService(QString databasePath);

    [[nodiscard]] BibleImportResult run(
        const BibleSource &source, const BibleImportOptions &options = {},
        const BibleImportProgressCallback &progress = {},
        const BibleImportCancellation &cancel = {}) const;

private:
    QString m_databasePath;
};

} // namespace churchpresenter
