#include "modules/BibleCommandModule.h"

#include <QSignalSpy>
#include <QTest>

using namespace churchpresenter;

class BibleCommandModuleTest final : public QObject {
    Q_OBJECT

private slots:
    void routesSearchAndPresentationCommands();
    void rejectsInvalidPresentationCoordinates();
};

void BibleCommandModuleTest::routesSearchAndPresentationCommands()
{
    qRegisterMetaType<DomainEvent>();
    CommandBus commands;
    EventBus events;
    QString reference;
    BibleCommandModule module(commands, events, {
        .search = [&reference](const QString &value) { reference = value; return true; },
        .present = [](int book, int chapter, int verse) {
            return book == 43 && chapter == 3 && verse == 16;
        },
        .stateSnapshot = [&reference] {
            return QVariantMap{{QStringLiteral("reference"), reference}};
        },
    });
    QSignalSpy eventSpy(&events, &EventBus::eventPublished);

    QVERIFY(module.requestSearch(QStringLiteral(" João 3:16 ")).accepted);
    QCOMPARE(reference, QStringLiteral("João 3:16"));
    QVERIFY(module.requestPresent(43, 3, 16, QStringLiteral("remote")).accepted);
    QCOMPARE(eventSpy.count(), 2);
    QCOMPARE(qvariant_cast<DomainEvent>(eventSpy.last().front()).type,
             QStringLiteral("bible.state.changed"));
}

void BibleCommandModuleTest::rejectsInvalidPresentationCoordinates()
{
    CommandBus commands;
    EventBus events;
    BibleCommandModule module(commands, events, {});
    QCOMPARE(module.requestPresent(0, 3, 16).errorCode, QStringLiteral("invalid_payload"));
    QCOMPARE(module.requestSearch(QStringLiteral("   ")).errorCode,
             QStringLiteral("invalid_payload"));
}

QTEST_APPLESS_MAIN(BibleCommandModuleTest)
#include "BibleCommandModuleTest.moc"
