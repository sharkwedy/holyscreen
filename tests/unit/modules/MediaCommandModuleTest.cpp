#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "modules/MediaCommandModule.h"

using namespace churchpresenter;

class MediaCommandModuleTest final : public QObject {
    Q_OBJECT

private slots:
    void routesOperatorAndRemoteThroughUnifiedPlayerCommands();
};

void MediaCommandModuleTest::routesOperatorAndRemoteThroughUnifiedPlayerCommands()
{
    qRegisterMetaType<DomainEvent>();
    CommandBus commandBus;
    EventBus eventBus;
    QString currentMedia;
    int positionMs = 0;
    MediaCommandModule module(commandBus, eventBus, {
        .play = [&currentMedia](const QString &id) { currentMedia = id; return true; },
        .togglePause = [] { return true; },
        .stop = [] { return true; },
        .seek = [&positionMs](int value) { positionMs = value; return true; },
        .previous = [] { return true; },
        .next = [] { return true; },
        .stateSnapshot = [&] {
            return QVariantMap{{QStringLiteral("mediaId"), currentMedia},
                               {QStringLiteral("positionMs"), positionMs}};
        },
    });
    QSignalSpy eventSpy(&eventBus, &EventBus::eventPublished);

    const auto playResult = module.requestPlay(QStringLiteral("audio-1"));
    const auto seekResult = commandBus.dispatch(Command{
        .id = QStringLiteral("remote-seek-1"),
        .type = QStringLiteral("media.seek"),
        .payload = {{QStringLiteral("positionMs"), 4200}},
        .source = QStringLiteral("remote"),
        .issuedAt = QDateTime::currentDateTimeUtc(),
    });

    QVERIFY(playResult.accepted);
    QVERIFY(seekResult.accepted);
    QCOMPARE(currentMedia, QStringLiteral("audio-1"));
    QCOMPARE(positionMs, 4200);
    QCOMPARE(eventSpy.count(), 2);
    const auto event = qvariant_cast<DomainEvent>(eventSpy.last().at(0));
    QCOMPARE(event.type, QStringLiteral("media.state.changed"));
    QCOMPARE(event.correlationId, QStringLiteral("remote-seek-1"));
    QCOMPARE(event.payload.value(QStringLiteral("positionMs")).toInt(), 4200);
}

QTEST_APPLESS_MAIN(MediaCommandModuleTest)
#include "MediaCommandModuleTest.moc"
