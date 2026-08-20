#pragma once

#include <QDateTime>
#include <QObject>
#include <QTimer>

#include <memory>

namespace churchpresenter {

class IClock {
public:
    virtual ~IClock() = default;
    [[nodiscard]] virtual QDateTime now() const = 0;
};

class SystemClock final : public IClock {
public:
    [[nodiscard]] QDateTime now() const override;
};

class ClockController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString text READ text NOTIFY textChanged)
    Q_PROPERTY(QString format READ format WRITE setFormat NOTIFY formatChanged)

public:
    explicit ClockController(std::unique_ptr<IClock> clock, QObject *parent = nullptr);

    [[nodiscard]] QString text() const;
    [[nodiscard]] QString format() const;
    void setFormat(const QString &format);
    Q_INVOKABLE void refresh();

signals:
    void textChanged();
    void formatChanged();

private:
    static QString qtFormat(const QString &format);

    std::unique_ptr<IClock> m_clock;
    QTimer m_timer;
    QString m_text;
    QString m_format = QStringLiteral("24h");
};

} // namespace churchpresenter
