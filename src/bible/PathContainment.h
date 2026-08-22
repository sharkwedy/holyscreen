#pragma once

#include <Qt>
#include <QString>

namespace churchpresenter {

[[nodiscard]] Qt::CaseSensitivity nativePathCaseSensitivity();
[[nodiscard]] bool isPathContained(const QString &root, const QString &candidate,
                                   Qt::CaseSensitivity caseSensitivity);
[[nodiscard]] bool isPathContained(const QString &root, const QString &candidate);

} // namespace churchpresenter
