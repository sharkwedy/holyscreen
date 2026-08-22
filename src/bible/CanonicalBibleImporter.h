#pragma once

#include "bible/IBibleSourceImporter.h"

namespace churchpresenter {

class CanonicalBibleImporter final : public IBibleSourceImporter {
public:
    [[nodiscard]] BibleImportPlan inspect(const BibleSource &source) const;
    [[nodiscard]] BibleImportPlan inspect(
        const BibleSource &source, const QString &localPath,
        const BibleImportProgressCallback &progress,
        const BibleImportCancellation &cancel) const override;
};

} // namespace churchpresenter
