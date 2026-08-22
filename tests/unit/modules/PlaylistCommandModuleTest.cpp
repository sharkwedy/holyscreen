#include "modules/PlaylistCommandModule.h"
#include <QTest>
using namespace churchpresenter;
class PlaylistCommandModuleTest final : public QObject {
    Q_OBJECT
private slots:
    void movesClearsAndRestores();
};
void PlaylistCommandModuleTest::movesClearsAndRestores(){CommandBus commands;EventBus events;UndoManager undo;QVariantList items{QVariantMap{{QStringLiteral("id"),QStringLiteral("a")}},QVariantMap{{QStringLiteral("id"),QStringLiteral("b")}}};PlaylistCommandModule module(commands,events,{.snapshot=[&]{return items;},.move=[&](const QString&id,int index){for(int i=0;i<items.size();++i)if(items[i].toMap().value(QStringLiteral("id")).toString()==id){items.move(i,index);return true;}return false;},.remove=[&](const QString&id){for(int i=0;i<items.size();++i)if(items[i].toMap().value(QStringLiteral("id")).toString()==id){items.removeAt(i);return true;}return false;},.clear=[&]{items.clear();return true;},.restore=[&](const QVariantList&snapshot){items=snapshot;return true;}},&undo);QVERIFY(module.requestMove(QStringLiteral("b"),0).accepted);QCOMPARE(items.front().toMap().value(QStringLiteral("id")).toString(),QStringLiteral("b"));QVERIFY(module.requestClear().accepted);QVERIFY(items.isEmpty());QVERIFY(undo.undo().success);QCOMPARE(items.size(),2);QVERIFY(undo.undo().success);QCOMPARE(items.front().toMap().value(QStringLiteral("id")).toString(),QStringLiteral("a"));}
QTEST_APPLESS_MAIN(PlaylistCommandModuleTest)
#include "PlaylistCommandModuleTest.moc"
