#pragma once

#include "bible/BibleImportTypes.h"

namespace churchpresenter {

class IBibleSourceImporter {
public:
    virtual ~IBibleSourceImporter() = default;
    [[nodiscard]] virtual BibleImportPlan inspect(
        const BibleSource &source, const QString &localPath,
        const BibleImportProgressCallback &progress,
        const BibleImportCancellation &cancel) const = 0;
};

} // namespace churchpresenter
