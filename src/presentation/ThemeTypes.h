#pragma once
#include <QString>
#include <QMetaType>

namespace churchpresenter {
enum class BackgroundType { SolidColor, Gradient, Image, Video, Transparent };
struct Theme {
    QString id;
    QString name;
    BackgroundType backgroundType = BackgroundType::SolidColor;
    QString backgroundColor = QStringLiteral("#000000");
    QString backgroundImage;
    QString fontFamily;
    int fontSize = 72;
    int minimumFontSize = 28;
    int fontWeight = 700;
    QString textColor = QStringLiteral("#ffffff");
    QString horizontalAlignment = QStringLiteral("center");
    QString verticalAlignment = QStringLiteral("center");
    int lineSpacing = 0;
    int margin = 64;
    bool outline = true;
    QString outlineColor = QStringLiteral("#a0000000");
    bool shadow = false;
    QString shadowColor = QStringLiteral("#80000000");
    QString transition = QStringLiteral("fade");
};
}
Q_DECLARE_METATYPE(churchpresenter::BackgroundType)
Q_DECLARE_METATYPE(churchpresenter::Theme)
