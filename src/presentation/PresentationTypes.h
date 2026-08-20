#pragma once

#include <QString>
#include <QVector>
#include <QMetaType>
#include <QStringList>

namespace churchpresenter {

enum class PresentationType { Song, Text, Bible };

struct Slide {
    QString id;
    QString label;
    QString text;
    int order = 0;
    QString themeOverride;
};

struct Presentation {
    QString id;
    PresentationType type = PresentationType::Text;
    QString title;
    QString author;
    QString defaultTheme;
    QVector<Slide> slides;
    QStringList sequence;
};

} // namespace churchpresenter

Q_DECLARE_METATYPE(churchpresenter::Slide)
Q_DECLARE_METATYPE(churchpresenter::Presentation)
