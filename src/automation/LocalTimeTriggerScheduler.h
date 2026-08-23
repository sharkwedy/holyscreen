#pragma once

#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

#include <functional>

namespace churchpresenter {

//! Produz no máximo um fato `time.local` por ocorrência de minuto local.
//! A chave inclui o offset UTC para distinguir horários repetidos no fim do DST.
class LocalTimeTriggerScheduler final : public QObject {
    Q_OBJECT

public:
    explicit LocalTimeTriggerScheduler(QObject *parent = nullptr);

    void setClock(std::function<QDateTime()> clock);
    void start();
    void stop();
    [[nodiscard]] bool isActive() const;

public slots:
    void checkNow();

signals:
    void localTimeOccurred(const QVariantMap &payload, const QString &correlationId);

private:
    QTimer m_timer;
    std::function<QDateTime()> m_clock;
    QString m_lastOccurrence;
};

} // namespace churchpresenter
