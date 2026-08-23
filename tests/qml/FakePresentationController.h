#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantMap>

//! Controlador falso usado pelos testes de componente QML. Expõe as mesmas
//! propriedades que as views de saída consomem, sem banco, telas ou mídia.
class FakePresentationController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool blackout MEMBER blackout NOTIFY changed)
    Q_PROPERTY(bool identifyVisible MEMBER identifyVisible NOTIFY changed)
    Q_PROPERTY(QString wallpaperColor MEMBER wallpaperColor NOTIFY changed)
    Q_PROPERTY(QUrl wallpaperSource MEMBER wallpaperSource NOTIFY changed)
    Q_PROPERTY(QString wallpaperFit MEMBER wallpaperFit NOTIFY changed)
    Q_PROPERTY(bool videoVisible MEMBER videoVisible NOTIFY changed)
    Q_PROPERTY(bool imageVisible MEMBER imageVisible NOTIFY changed)
    Q_PROPERTY(QUrl presentationImageSource MEMBER presentationImageSource NOTIFY changed)
    Q_PROPERTY(QString imageFit MEMBER imageFit NOTIFY changed)
    Q_PROPERTY(QString imageTransition MEMBER imageTransition NOTIFY changed)
    Q_PROPERTY(bool textVisible MEMBER textVisible NOTIFY changed)
    Q_PROPERTY(QVariantMap activeTheme MEMBER activeTheme NOTIFY changed)
    Q_PROPERTY(QString currentPresentationType MEMBER currentPresentationType NOTIFY changed)
    Q_PROPERTY(int currentSlideIndex MEMBER currentSlideIndex NOTIFY changed)
    Q_PROPERTY(QString currentSlideText MEMBER currentSlideText NOTIFY changed)
    Q_PROPERTY(QString nextSlideText MEMBER nextSlideText NOTIFY changed)
    Q_PROPERTY(QString stageMessage MEMBER stageMessage NOTIFY changed)
    Q_PROPERTY(QString audienceMessage MEMBER audienceMessage NOTIFY changed)
    Q_PROPERTY(QString alertMessage MEMBER alertMessage NOTIFY changed)
    Q_PROPERTY(QString lowerThirdTitle MEMBER lowerThirdTitle NOTIFY changed)
    Q_PROPERTY(QString lowerThirdSubtitle MEMBER lowerThirdSubtitle NOTIFY changed)
    Q_PROPERTY(QString countdownText MEMBER countdownText NOTIFY changed)
    Q_PROPERTY(bool countdownRunning MEMBER countdownRunning NOTIFY changed)
    Q_PROPERTY(QString stopwatchText MEMBER stopwatchText NOTIFY changed)
    Q_PROPERTY(bool stopwatchRunning MEMBER stopwatchRunning NOTIFY changed)
    Q_PROPERTY(bool clockVisible MEMBER clockVisible NOTIFY changed)
    Q_PROPERTY(QString clockText MEMBER clockText NOTIFY changed)
    Q_PROPERTY(QString clockPosition MEMBER clockPosition NOTIFY changed)
    Q_PROPERTY(QString clockFontFamily MEMBER clockFontFamily NOTIFY changed)
    Q_PROPERTY(int clockFontSize MEMBER clockFontSize NOTIFY changed)
    Q_PROPERTY(QString clockColor MEMBER clockColor NOTIFY changed)
    Q_PROPERTY(bool clockFontBold MEMBER clockFontBold NOTIFY changed)
    Q_PROPERTY(bool clockFontItalic MEMBER clockFontItalic NOTIFY changed)
    Q_PROPERTY(QString clockBackgroundColor MEMBER clockBackgroundColor NOTIFY changed)
    Q_PROPERTY(double clockLineHeight MEMBER clockLineHeight NOTIFY changed)
    Q_PROPERTY(int clockCornerRadius MEMBER clockCornerRadius NOTIFY changed)
    Q_PROPERTY(double clockTextOpacity MEMBER clockTextOpacity NOTIFY changed)
    Q_PROPERTY(double clockBackgroundOpacity MEMBER clockBackgroundOpacity NOTIFY changed)
    Q_PROPERTY(int clockMarginHorizontal MEMBER clockMarginHorizontal NOTIFY changed)
    Q_PROPERTY(int clockMarginVertical MEMBER clockMarginVertical NOTIFY changed)
    Q_PROPERTY(QString clockEffect MEMBER clockEffect NOTIFY changed)
    Q_PROPERTY(int registeredSinks MEMBER registeredSinks NOTIFY changed)

public:
    using QObject::QObject;

    Q_INVOKABLE QString bibleTextForSlide(int index, const QString &translationId) const
    {
        return QStringLiteral("%1#%2").arg(translationId).arg(index);
    }
    Q_INVOKABLE void registerVideoSink(const QVariant &) { ++registeredSinks; emit changed(); }
    Q_INVOKABLE void unregisterVideoSink(const QVariant &) { --registeredSinks; emit changed(); }

    bool blackout = false;
    bool identifyVisible = false;
    QString wallpaperColor = QStringLiteral("#101010");
    QUrl wallpaperSource;
    QString wallpaperFit = QStringLiteral("cover");
    bool videoVisible = false;
    bool imageVisible = false;
    QUrl presentationImageSource;
    QString imageFit = QStringLiteral("contain");
    QString imageTransition = QStringLiteral("none");
    bool textVisible = false;
    QVariantMap activeTheme{
        {QStringLiteral("backgroundType"), 0},
        {QStringLiteral("backgroundColor"), QStringLiteral("#000000")},
        {QStringLiteral("backgroundImage"), QString{}},
        {QStringLiteral("textColor"), QStringLiteral("#ffffff")},
        {QStringLiteral("transition"), QStringLiteral("none")},
        {QStringLiteral("margin"), 64},
        {QStringLiteral("fontSize"), 72},
        {QStringLiteral("minimumFontSize"), 28},
        {QStringLiteral("fontWeight"), 700},
        {QStringLiteral("lineSpacing"), 0},
        {QStringLiteral("horizontalAlignment"), QStringLiteral("center")},
        {QStringLiteral("verticalAlignment"), QStringLiteral("center")},
        {QStringLiteral("outline"), false},
        {QStringLiteral("shadow"), false},
        {QStringLiteral("outlineColor"), QStringLiteral("#000000")},
        {QStringLiteral("shadowColor"), QStringLiteral("#000000")},
    };
    QString currentPresentationType = QStringLiteral("song");
    int currentSlideIndex = 0;
    QString currentSlideText;
    QString nextSlideText;
    QString stageMessage;
    QString audienceMessage;
    QString alertMessage;
    QString lowerThirdTitle;
    QString lowerThirdSubtitle;
    QString countdownText = QStringLiteral("00:00");
    bool countdownRunning = false;
    QString stopwatchText = QStringLiteral("00:00");
    bool stopwatchRunning = false;
    bool clockVisible = false;
    QString clockText = QStringLiteral("10:00");
    QString clockPosition = QStringLiteral("bottomRight");
    QString clockFontFamily;
    int clockFontSize = 48;
    QString clockColor = QStringLiteral("#ffffff");
    bool clockFontBold = true;
    bool clockFontItalic = false;
    QString clockBackgroundColor = QStringLiteral("#000000");
    double clockLineHeight = 1.0;
    int clockCornerRadius = 12;
    double clockTextOpacity = 1.0;
    double clockBackgroundOpacity = 0.5;
    int clockMarginHorizontal = 0;
    int clockMarginVertical = 0;
    QString clockEffect = QStringLiteral("outline");
    int registeredSinks = 0;

signals:
    void changed();
};
