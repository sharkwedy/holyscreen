#pragma once

#include "bible/BibleTypes.h"

#include <optional>

namespace churchpresenter {

class BibleReferenceParser final {
public:
    [[nodiscard]] std::optional<BibleReference> parse(const QString &text) const;
};

} // namespace churchpresenter
