#include "remote/RemoteAuthManager.h"

#include <QTest>

using namespace churchpresenter;

class RemoteAuthManagerTest final : public QObject {
    Q_OBJECT
private slots:
    void storesPbkdf2CredentialsAndIssuesHashedSessions();
    void expiresAndRevokesSessions();
    void blocksBruteForceAndLimitsCommands();
};

void RemoteAuthManagerTest::storesPbkdf2CredentialsAndIssuesHashedSessions()
{
    RemoteAuthManager auth;
    const auto credentials = auth.setPassword(QStringLiteral("senha-local-forte"), 1000);
    QCOMPARE(credentials.value(QStringLiteral("algorithm")).toString(),
             QStringLiteral("PBKDF2-HMAC-SHA256"));
    QCOMPARE(credentials.value(QStringLiteral("salt")).toByteArray().size(), 16);
    QCOMPARE(credentials.value(QStringLiteral("hash")).toByteArray().size(), 32);
    QVERIFY(!credentials.values().contains(QStringLiteral("senha-local-forte")));
    QVERIFY(auth.hasCredentials());
    QVERIFY(!auth.login(QStringLiteral("errada"), QStringLiteral("127.0.0.1")).accepted);
    const auto session = auth.login(QStringLiteral("senha-local-forte"), QStringLiteral("127.0.0.1"));
    QVERIFY(session.accepted);
    QVERIFY(session.token.size() >= 40);
    QVERIFY(auth.validateToken(session.token));
    QCOMPARE(auth.sessionCount(), 1);
}

void RemoteAuthManagerTest::expiresAndRevokesSessions()
{
    auto now = QDateTime::fromString(QStringLiteral("2026-08-22T12:00:00Z"), Qt::ISODate);
    RemoteAuthManager auth([&now] { return now; });
    QVERIFY(!auth.setPassword(QStringLiteral("senha"), 1000).isEmpty());
    const auto first = auth.login(QStringLiteral("senha"), QStringLiteral("client"));
    QVERIFY(auth.logout(first.token));
    QVERIFY(!auth.validateToken(first.token));
    const auto second = auth.login(QStringLiteral("senha"), QStringLiteral("client"));
    const auto secondHash = auth.sessionHash(second.token);
    QVERIFY(auth.validateSession(secondHash));
    now = now.addSecs(RemoteAuthManager::SessionHours * 3600 + 1);
    QVERIFY(!auth.validateToken(second.token));
    QVERIFY(!auth.validateSession(secondHash));
    QCOMPARE(auth.sessionCount(), 0);
}

void RemoteAuthManagerTest::blocksBruteForceAndLimitsCommands()
{
    auto now = QDateTime::fromString(QStringLiteral("2026-08-22T12:00:00Z"), Qt::ISODate);
    RemoteAuthManager auth([&now] { return now; });
    QVERIFY(!auth.setPassword(QStringLiteral("senha"), 1000).isEmpty());
    for (int attempt = 0; attempt < 5; ++attempt)
        QVERIFY(!auth.login(QStringLiteral("errada"), QStringLiteral("client-a")).accepted);
    QCOMPARE(auth.login(QStringLiteral("senha"), QStringLiteral("client-a")).errorCode,
             QStringLiteral("login_blocked"));
    now = now.addSecs(15 * 60 + 1);
    const auto session = auth.login(QStringLiteral("senha"), QStringLiteral("client-a"));
    QVERIFY(session.accepted);
    for (int command = 0; command < 30; ++command) QVERIFY(auth.authorizeCommand(session.token));
    QVERIFY(!auth.authorizeCommand(session.token));
    now = now.addSecs(1);
    QVERIFY(auth.authorizeCommand(session.token));
}

QTEST_APPLESS_MAIN(RemoteAuthManagerTest)
#include "RemoteAuthManagerTest.moc"
