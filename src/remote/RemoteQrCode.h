#pragma once

#include <QString>

namespace churchpresenter {

class RemoteQrCode final {
public:
    [[nodiscard]] static QString svgDataUrl(const QString &text);
};

} // namespace churchpresenter
