#include "core/CommandCatalog.h"

#include <QSet>
#include <QTest>

using namespace churchpresenter;

class CommandCatalogTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesUniqueOperationalCommands();
    void limitsRemoteCommandsToTheExplicitAllowlist();
};

void CommandCatalogTest::exposesUniqueOperationalCommands()
{
    const auto descriptors = CommandCatalog::descriptors();
    QVERIFY(!descriptors.isEmpty());
    QSet<QString> unique;
    for (const auto &descriptor : descriptors) {
        QVERIFY(!descriptor.type.trimmed().isEmpty());
        QVERIFY(!unique.contains(descriptor.type));
        unique.insert(descriptor.type);
        QVERIFY(CommandCatalog::contains(descriptor.type));
    }
}

void CommandCatalogTest::limitsRemoteCommandsToTheExplicitAllowlist()
{
    QVERIFY(CommandCatalog::isRemoteAllowed(QStringLiteral("presentation.slide.next")));
    QVERIFY(CommandCatalog::isRemoteAllowed(QStringLiteral("media.play")));
    QVERIFY(CommandCatalog::isRemoteAllowed(QStringLiteral("bible.reference.present")));
    QVERIFY(CommandCatalog::isRemoteAllowed(QStringLiteral("event.item.execute")));
    QVERIFY(CommandCatalog::isRemoteAllowed(QStringLiteral("stage.message.set")));
    QVERIFY(!CommandCatalog::isRemoteAllowed(QStringLiteral("settings.theme.apply")));
    QVERIFY(!CommandCatalog::isRemoteAllowed(QStringLiteral("unknown.command")));
}

QTEST_APPLESS_MAIN(CommandCatalogTest)
#include "CommandCatalogTest.moc"
