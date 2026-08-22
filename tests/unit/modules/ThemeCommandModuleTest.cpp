#include "modules/ThemeCommandModule.h"
#include <QTest>
using namespace churchpresenter;

class ThemeCommandModuleTest final : public QObject {
    Q_OBJECT
private slots:
    void appliesAndReverts();
};

void ThemeCommandModuleTest::appliesAndReverts()
{
    CommandBus commands;
    EventBus events;
    UndoManager undo;
    QString current = QStringLiteral("dark");
    ThemeCommandModule module(commands, events, {
        .currentThemeId = [&] { return current; },
        .apply = [&](const QString &id) {
            if (id != QStringLiteral("dark") && id != QStringLiteral("light")) return false;
            current = id;
            return true;
        },
        .stateSnapshot = [&] {
            return QVariantMap{{QStringLiteral("themeId"), current}};
        },
    }, &undo);
    QVERIFY(module.requestApply(QStringLiteral("light")).accepted);
    QCOMPARE(current, QStringLiteral("light"));
    QVERIFY(undo.undo().success);
    QCOMPARE(current, QStringLiteral("dark"));
    QVERIFY(undo.redo().success);
    QCOMPARE(current, QStringLiteral("light"));
    QCOMPARE(module.requestApply(QString{}).errorCode, QStringLiteral("invalid_payload"));
}
QTEST_APPLESS_MAIN(ThemeCommandModuleTest)
#include "ThemeCommandModuleTest.moc"
