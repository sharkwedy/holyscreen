#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "screens/ScreenManager.h"

using namespace churchpresenter;

class FakeScreenProvider final : public IScreenProvider {
public:
    QVector<ScreenDescriptor> values;

    QVector<ScreenDescriptor> screens() const override { return values; }
    void publish(QVector<ScreenDescriptor> updated)
    {
        values = std::move(updated);
        emit screensChanged();
    }
};

class ScreenManagerTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesAllScreensFromProvider();
    void emitsConnectionAndDisconnectionEvents();
    void emitsConfigurationChangeForResolutionChanges();
};

static ScreenDescriptor display(const QString &fingerprint, const QSize &resolution = {1920, 1080})
{
    return ScreenDescriptor{
        .id = fingerprint,
        .fingerprint = fingerprint,
        .displayName = fingerprint,
        .geometry = QRect(QPoint{}, resolution),
        .resolution = resolution,
        .connected = true,
    };
}

void ScreenManagerTest::exposesAllScreensFromProvider()
{
    FakeScreenProvider provider;
    provider.values = {display("laptop"), display("projector")};
    ScreenManager manager(provider);

    QCOMPARE(manager.screens().size(), 2);
}

void ScreenManagerTest::emitsConnectionAndDisconnectionEvents()
{
    FakeScreenProvider provider;
    provider.values = {display("laptop")};
    ScreenManager manager(provider);
    QSignalSpy connected(&manager, &ScreenManager::screenConnected);
    QSignalSpy disconnected(&manager, &ScreenManager::screenDisconnected);

    provider.publish({display("laptop"), display("projector")});
    QCOMPARE(connected.size(), 1);
    QCOMPARE(connected.front().front().value<ScreenDescriptor>().fingerprint, QStringLiteral("projector"));

    provider.publish({display("laptop")});
    QCOMPARE(disconnected.size(), 1);
    QCOMPARE(disconnected.front().front().toString(), QStringLiteral("projector"));
}

void ScreenManagerTest::emitsConfigurationChangeForResolutionChanges()
{
    FakeScreenProvider provider;
    provider.values = {display("projector")};
    ScreenManager manager(provider);
    QSignalSpy changed(&manager, &ScreenManager::screenConfigurationChanged);

    provider.publish({display("projector", {3840, 2160})});

    QCOMPARE(changed.size(), 1);
    QCOMPARE(manager.screens().front().resolution, QSize(3840, 2160));
}

QTEST_APPLESS_MAIN(ScreenManagerTest)
#include "ScreenManagerTest.moc"
