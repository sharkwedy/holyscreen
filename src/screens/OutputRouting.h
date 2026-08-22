#pragma once

#include "screens/OutputManager.h"

#include <QRect>
#include <QString>
#include <QVector>

namespace churchpresenter {

struct OutputPlacement {
    QString screenFingerprint;
    QString screenId;
    QString displayName;
    qsizetype screenIndex = -1;
    QRect geometry;
    QString bibleTranslationId;
    OutputRole role = OutputRole::Audience;
    bool mediaEnabled = true;
};

[[nodiscard]] QVector<OutputPlacement> routeOutputs(
    const QVector<OutputDescriptor> &outputs,
    const QVector<ScreenDescriptor> &screens);

[[nodiscard]] QVector<OutputPlacement> routeAudienceOutputs(
    const QVector<OutputDescriptor> &outputs,
    const QVector<ScreenDescriptor> &screens);

} // namespace churchpresenter
