#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

namespace churchpresenter {

class TimedMediaPlayback final : public QObject {
    Q_OBJECT

public:
    explicit TimedMediaPlayback(QObject *parent = nullptr);

    [[nodiscard]] bool active() const;
    [[nodiscard]] bool playing() const;
    [[nodiscard]] int positionMs() const;
    [[nodiscard]] int durationMs() const;

    void start(int durationMs);
    void pause();
    void resume();
    void stop();
    void seek(int positionMs);

signals:
    void stateChanged();
    void positionChanged();
    void finished();

private:
    void tick();

    QTimer m_timer;
    QElapsedTimer m_elapsed;
    int m_basePositionMs = 0;
    int m_durationMs = 0;
    bool m_active = false;
    bool m_playing = false;
};

} // namespace churchpresenter
