#pragma once

#include <QObject>
#include <QRect>
#include <QSize>
#include <QString>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QScreen)

namespace churchpresenter {

struct ScreenDescriptor {
    QString id;
    QString fingerprint;
    QString displayName;
    QRect geometry;
    QSize resolution;
    qreal devicePixelRatio = 1.0;
    bool primary = false;
    bool connected = false;
    QScreen *nativeScreen = nullptr;
};

class IScreenProvider : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IScreenProvider() override = default;
    [[nodiscard]] virtual QVector<ScreenDescriptor> screens() const = 0;

signals:
    void screensChanged();
};

class ScreenManager final : public QObject {
    Q_OBJECT

public:
    explicit ScreenManager(IScreenProvider &provider, QObject *parent = nullptr);

    [[nodiscard]] const QVector<ScreenDescriptor> &screens() const;
    void refresh();

signals:
    void screenConnected(const churchpresenter::ScreenDescriptor &screen);
    void screenDisconnected(const QString &fingerprint);
    void screenConfigurationChanged();

private:
    IScreenProvider &m_provider;
    QVector<ScreenDescriptor> m_screens;
};

class QtScreenProvider final : public IScreenProvider {
    Q_OBJECT

public:
    explicit QtScreenProvider(QObject *parent = nullptr);
    [[nodiscard]] QVector<ScreenDescriptor> screens() const override;

private:
    static QString fingerprintFor(QScreen *screen);
    void watch(QScreen *screen);
};

} // namespace churchpresenter

Q_DECLARE_METATYPE(churchpresenter::ScreenDescriptor)
