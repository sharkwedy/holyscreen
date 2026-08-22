#include "bible/PathContainment.h"

#include <QTest>

using namespace churchpresenter;

class PathContainmentTest final : public QObject {
    Q_OBJECT

private slots:
    void acceptsMixedWindowsSeparatorsAndCase();
    void rejectsSiblingPrefixesAndTraversal();
};

void PathContainmentTest::acceptsMixedWindowsSeparatorsAndCase()
{
    QVERIFY(isPathContained(QStringLiteral("C:/HolyScreen/staging"),
                            QStringLiteral("c:\\holyscreen\\staging\\data/canonical/TB/meta.json"),
                            Qt::CaseInsensitive));
    QVERIFY(isPathContained(QStringLiteral("C:\\HolyScreen\\staging"),
                            QStringLiteral("C:/HolyScreen/staging"),
                            Qt::CaseInsensitive));
}

void PathContainmentTest::rejectsSiblingPrefixesAndTraversal()
{
    QVERIFY(!isPathContained(QStringLiteral("C:/HolyScreen/staging"),
                             QStringLiteral("C:/HolyScreen/staging-other/file.json"),
                             Qt::CaseInsensitive));
    QVERIFY(!isPathContained(QStringLiteral("/tmp/staging"),
                             QStringLiteral("/tmp/staging/../escape.json"),
                             Qt::CaseSensitive));
}

QTEST_APPLESS_MAIN(PathContainmentTest)
#include "PathContainmentTest.moc"
