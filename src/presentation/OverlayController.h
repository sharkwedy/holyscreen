#pragma once

#include <QObject>
#include <QTimer>

namespace churchpresenter {

class OverlayController final : public QObject {
    Q_OBJECT
public:
    explicit OverlayController(QObject *parent = nullptr);

    [[nodiscard]] QString message() const;
    [[nodiscard]] QString alert() const;
    [[nodiscard]] QString lowerThirdTitle() const;
    [[nodiscard]] QString lowerThirdSubtitle() const;
    [[nodiscard]] QString countdownText() const;
    [[nodiscard]] QString stopwatchText() const;
    [[nodiscard]] bool countdownRunning() const;
    [[nodiscard]] bool stopwatchRunning() const;

    void setMessage(const QString &message);
    void setAlert(const QString &alert);
    void setLowerThird(const QString &title, const QString &subtitle);
    void startCountdown(int seconds);
    void stopCountdown();
    void startStopwatch();
    void pauseStopwatch();
    void resetStopwatch();
    void advanceOneSecond();

signals:
    void changed();
    void countdownExpired();

private:
    QTimer m_tick;
    QString m_message;
    QString m_alert;
    QString m_lowerThirdTitle;
    QString m_lowerThirdSubtitle;
    int m_countdownSeconds = 0;
    int m_stopwatchSeconds = 0;
    bool m_countdownRunning = false;
    bool m_stopwatchRunning = false;
};

} // namespace churchpresenter
