#include "presentation/PresentationController.h"

namespace churchpresenter {

void PresentationController::setWallpaperColor(const QString &color) { m_wallpaperColor = color; }
void PresentationController::setBlackout(bool enabled) { m_blackout = enabled; }
void PresentationController::restorePresentation() { m_blackout = false; }
void PresentationController::setClockVisible(bool visible) { m_clockVisible = visible; }
QString PresentationController::wallpaperColor() const { return m_wallpaperColor; }
QString PresentationController::visibleBackgroundColor() const { return m_blackout ? QStringLiteral("#000000") : m_wallpaperColor; }
bool PresentationController::blackout() const { return m_blackout; }
bool PresentationController::clockVisible() const { return m_clockVisible; }

} // namespace churchpresenter
