#include "remote/LocalApiServer.h"

#include <QSignalSpy>
#include <QTest>

using namespace churchpresenter;

class LocalApiServerTest final : public QObject {
    Q_OBJECT
private slots:
    void validatesAndDispatchesJsonCommands();
    void startsOnAnAvailableLocalPortAndServesTheRemotePage();
    void reportsWhyAPortCannotBeOpened();
};

void LocalApiServerTest::validatesAndDispatchesJsonCommands()
{
    LocalApiServer server;
    QSignalSpy commands(&server, &LocalApiServer::commandReceived);
    QVERIFY(!server.processCommandPayload("not-json"));
    QVERIFY(!server.processCommandPayload("{}"));
    QVERIFY(server.processCommandPayload(R"({"command":"presentation.next","arguments":{"source":"test"}})"));
    QCOMPARE(commands.count(), 1);
    QCOMPARE(commands.front().front().toString(), QStringLiteral("presentation.next"));
}

void LocalApiServerTest::startsOnAnAvailableLocalPortAndServesTheRemotePage()
{
    LocalApiServer server;
    QVERIFY(server.start(0));
    QVERIFY(server.running());
    QVERIFY(server.port() > 0);
    QVERIFY(server.remoteUrl().startsWith(QStringLiteral("http://")));
    QVERIFY(LocalApiServer::remotePage().contains("presentation.next"));
    server.stop();
    QVERIFY(!server.running());
}

void LocalApiServerTest::reportsWhyAPortCannotBeOpened()
{
    QTcpServer occupiedPort;
    QVERIFY(occupiedPort.listen(QHostAddress::AnyIPv4, 0));
    LocalApiServer server;

    QVERIFY(!server.start(occupiedPort.serverPort()));
    QVERIFY2(!server.lastError().isEmpty(), "A falha de bind deve expor um diagnóstico acionável");
    QVERIFY(server.lastError().contains(QString::number(occupiedPort.serverPort())));
}

QTEST_MAIN(LocalApiServerTest)
#include "LocalApiServerTest.moc"
