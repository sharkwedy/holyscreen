#pragma once

#include <QString>

namespace churchpresenter {

class PresentationController {
public:
    void setWallpaperColor(const QString &color);
    void setBlackout(bool enabled);
    void restorePresentation();
    void setClockVisible(bool visible);

    [[nodiscard]] QString wallpaperColor() const;
    [[nodiscard]] QString visibleBackgroundColor() const;
    [[nodiscard]] bool blackout() const;
    [[nodiscard]] bool clockVisible() const;

private:
    QString m_wallpaperColor = QStringLiteral("#000000");
    bool m_blackout = false;
    bool m_clockVisible = true;
};

} // namespace churchpresenter
