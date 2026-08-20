#include "presentation/ImagePresentationController.h"

#include <algorithm>

namespace churchpresenter {

ImagePresentationController::ImagePresentationController(QObject *parent)
    : QObject(parent)
{
    m_autoplayTimer.setSingleShot(false);
    m_autoplayTimer.setInterval(m_autoplayIntervalMs);
    connect(&m_autoplayTimer, &QTimer::timeout, this, &ImagePresentationController::next);
}

void ImagePresentationController::setPlaylist(QVector<MediaItem> images)
{
    const auto currentId = current().id;
    m_playlist = std::move(images);
    m_currentIndex = -1;
    if (!currentId.isEmpty()) {
        for (int index = 0; index < m_playlist.size(); ++index) {
            if (m_playlist[index].id == currentId) {
                m_currentIndex = index;
                break;
            }
        }
    }
    if (m_currentIndex < 0 && m_visible) stop();
}

const QVector<MediaItem> &ImagePresentationController::playlist() const { return m_playlist; }

MediaItem ImagePresentationController::current() const
{
    return m_currentIndex >= 0 && m_currentIndex < m_playlist.size()
        ? m_playlist[m_currentIndex]
        : MediaItem{};
}

bool ImagePresentationController::visible() const { return m_visible; }

bool ImagePresentationController::show(const QString &mediaId)
{
    for (int index = 0; index < m_playlist.size(); ++index) {
        if (m_playlist[index].id == mediaId) {
            selectIndex(index);
            if (!m_visible) {
                m_visible = true;
                emit visibleChanged(true);
            }
            restartAutoplayTimer();
            return true;
        }
    }
    return false;
}

void ImagePresentationController::next()
{
    if (m_playlist.isEmpty()) return;
    selectIndex(m_currentIndex < 0 ? 0 : (m_currentIndex + 1) % m_playlist.size());
    if (!m_visible) {
        m_visible = true;
        emit visibleChanged(true);
    }
    restartAutoplayTimer();
}

void ImagePresentationController::previous()
{
    if (m_playlist.isEmpty()) return;
    selectIndex(m_currentIndex < 0 ? m_playlist.size() - 1
                                  : (m_currentIndex - 1 + m_playlist.size()) % m_playlist.size());
    if (!m_visible) {
        m_visible = true;
        emit visibleChanged(true);
    }
    restartAutoplayTimer();
}

void ImagePresentationController::stop()
{
    m_autoplayTimer.stop();
    if (!m_visible) return;
    m_visible = false;
    emit visibleChanged(false);
}

void ImagePresentationController::setFit(ImageFit fit)
{
    if (m_fit == fit) return;
    m_fit = fit;
    emit fitChanged(fit);
}

ImageFit ImagePresentationController::fit() const { return m_fit; }

void ImagePresentationController::setTransition(ImageTransition transition)
{
    if (m_transition == transition) return;
    m_transition = transition;
    emit transitionChanged(transition);
}

ImageTransition ImagePresentationController::transition() const { return m_transition; }

void ImagePresentationController::setAutoplay(bool enabled)
{
    if (m_autoplay == enabled) return;
    m_autoplay = enabled;
    emit autoplayChanged(enabled);
    restartAutoplayTimer();
}

bool ImagePresentationController::autoplay() const { return m_autoplay; }

void ImagePresentationController::setAutoplayIntervalMs(int intervalMs)
{
    const auto normalized = std::clamp(intervalMs, 250, 3'600'000);
    if (m_autoplayIntervalMs == normalized) return;
    m_autoplayIntervalMs = normalized;
    m_autoplayTimer.setInterval(normalized);
    emit autoplayIntervalChanged(normalized);
    restartAutoplayTimer();
}

int ImagePresentationController::autoplayIntervalMs() const { return m_autoplayIntervalMs; }
bool ImagePresentationController::autoplayTimerActive() const { return m_autoplayTimer.isActive(); }

void ImagePresentationController::restartAutoplayTimer()
{
    if (m_autoplay && m_visible && m_playlist.size() > 1) {
        m_autoplayTimer.start(m_autoplayIntervalMs);
    } else {
        m_autoplayTimer.stop();
    }
}

void ImagePresentationController::selectIndex(int index)
{
    if (index < 0 || index >= m_playlist.size()) return;
    m_currentIndex = index;
    emit currentChanged(m_playlist[index]);
}

} // namespace churchpresenter
