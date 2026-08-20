#pragma once

#include "presentation/MediaTypes.h"

#include <QObject>
#include <QTimer>
#include <QVector>

namespace churchpresenter {

enum class ImageFit {
    Contain,
    Cover,
    Stretch,
    Center,
};

enum class ImageTransition {
    None,
    Fade,
};

class ImagePresentationController final : public QObject {
    Q_OBJECT

public:
    explicit ImagePresentationController(QObject *parent = nullptr);

    void setPlaylist(QVector<MediaItem> images);
    [[nodiscard]] const QVector<MediaItem> &playlist() const;
    [[nodiscard]] MediaItem current() const;
    [[nodiscard]] bool visible() const;

    bool show(const QString &mediaId);
    void next();
    void previous();
    void stop();

    void setFit(ImageFit fit);
    [[nodiscard]] ImageFit fit() const;
    void setTransition(ImageTransition transition);
    [[nodiscard]] ImageTransition transition() const;
    void setAutoplay(bool enabled);
    [[nodiscard]] bool autoplay() const;
    void setAutoplayIntervalMs(int intervalMs);
    [[nodiscard]] int autoplayIntervalMs() const;
    [[nodiscard]] bool autoplayTimerActive() const;

signals:
    void currentChanged(const churchpresenter::MediaItem &item);
    void visibleChanged(bool visible);
    void fitChanged(churchpresenter::ImageFit fit);
    void transitionChanged(churchpresenter::ImageTransition transition);
    void autoplayChanged(bool enabled);
    void autoplayIntervalChanged(int intervalMs);

private:
    void restartAutoplayTimer();
    void selectIndex(int index);

    QVector<MediaItem> m_playlist;
    int m_currentIndex = -1;
    bool m_visible = false;
    ImageFit m_fit = ImageFit::Contain;
    ImageTransition m_transition = ImageTransition::Fade;
    bool m_autoplay = false;
    int m_autoplayIntervalMs = 5000;
    QTimer m_autoplayTimer;
};

} // namespace churchpresenter

Q_DECLARE_METATYPE(churchpresenter::ImageFit)
Q_DECLARE_METATYPE(churchpresenter::ImageTransition)
