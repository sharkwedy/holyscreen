#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

namespace churchpresenter {

class ApplicationController;

class EventContext final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList events READ events NOTIFY eventsChanged)
    Q_PROPERTY(QString currentEventId READ currentEventId NOTIFY currentEventChanged)
    Q_PROPERTY(QVariantList eventItems READ eventItems NOTIFY eventItemsChanged)
    Q_PROPERTY(qint64 eventDurationMs READ eventDurationMs NOTIFY eventItemsChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)
    Q_PROPERTY(QVariantMap historyReport READ historyReport NOTIFY historyChanged)

public:
    explicit EventContext(ApplicationController &controller, QObject *parent = nullptr);

    [[nodiscard]] QVariantList events() const;
    [[nodiscard]] QString currentEventId() const;
    [[nodiscard]] QVariantList eventItems() const;
    [[nodiscard]] qint64 eventDurationMs() const;
    [[nodiscard]] QVariantList history() const;
    [[nodiscard]] QVariantMap historyReport() const;

    Q_INVOKABLE QString createEvent(const QString &title, const QString &scheduledAt);
    Q_INVOKABLE void selectEvent(const QString &id);
    Q_INVOKABLE void deleteEvent(const QString &id);
    Q_INVOKABLE void addEventItem(const QString &type, const QString &referenceId,
                                  const QString &title, qint64 durationMs = 0);
    Q_INVOKABLE void removeEventItem(const QString &id);
    Q_INVOKABLE void moveEventItem(const QString &id, int newIndex);
    Q_INVOKABLE void executeEventItem(const QString &id);
    Q_INVOKABLE void clearHistory();

signals:
    void eventsChanged();
    void currentEventChanged();
    void eventItemsChanged();
    void historyChanged();

private:
    ApplicationController &m_controller;
};

} // namespace churchpresenter
