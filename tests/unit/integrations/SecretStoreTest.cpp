#include "integrations/secrets/InMemorySecretStore.h"
#include "integrations/secrets/SecretStoreFactory.h"

#include <QTest>
#include <QUuid>

#if defined(Q_OS_MACOS)
#include "integrations/secrets/KeychainSecretStore.h"
#endif

using namespace churchpresenter;

class SecretStoreTest final : public QObject {
    Q_OBJECT

private slots:
    void inMemoryStoreKeepsSecretsOnlyForThisSession();
    void factoryAlwaysReturnsAUsableStore();
    void forcesTheMemoryStoreWhenTheEnvironmentAsksForIt();
    void systemStoreRoundTripsASecret();
};

void SecretStoreTest::inMemoryStoreKeepsSecretsOnlyForThisSession()
{
    InMemorySecretStore store;
    QVERIFY(!store.isPersistent());
    QVERIFY(!store.backendName().isEmpty());

    QVERIFY(!store.store(QStringLiteral("   "), QStringLiteral("x")));
    QVERIFY(store.store(QStringLiteral("obs/password"), QStringLiteral("segredo")));
    QCOMPARE(store.retrieve(QStringLiteral("obs/password")),
             std::optional<QString>{QStringLiteral("segredo")});
    QCOMPARE(store.references(), QStringList{QStringLiteral("obs/password")});
    QVERIFY(store.remove(QStringLiteral("obs/password")));
    QVERIFY(!store.remove(QStringLiteral("obs/password")));
    QCOMPARE(store.retrieve(QStringLiteral("obs/password")), std::nullopt);
}

void SecretStoreTest::factoryAlwaysReturnsAUsableStore()
{
    const auto store = SecretStoreFactory::create();
    QVERIFY(store != nullptr);
    QVERIFY(!store->backendName().isEmpty());
    QVERIFY(!SecretStoreFactory::platformBackendName().isEmpty());
    // Sem cofre do sistema a fábrica precisa avisar que nada será persistido,
    // em vez de gravar em texto puro.
    if (!store->isPersistent()) {
        QCOMPARE(store->backendName(), QStringLiteral("memória (somente nesta sessão)"));
    }
}

void SecretStoreTest::forcesTheMemoryStoreWhenTheEnvironmentAsksForIt()
{
    qputenv("HOLYSCREEN_SECRET_STORE", QByteArrayLiteral("memory"));
    const auto store = SecretStoreFactory::create();
    QVERIFY(!store->isPersistent());
    qunsetenv("HOLYSCREEN_SECRET_STORE");
}

void SecretStoreTest::systemStoreRoundTripsASecret()
{
    if (qEnvironmentVariableIsEmpty("HOLYSCREEN_TEST_SYSTEM_SECRET_STORE")) {
        QSKIP("Defina HOLYSCREEN_TEST_SYSTEM_SECRET_STORE=1 para exercitar o cofre do sistema. "
              "O acesso ao cofre pode exigir desbloqueio interativo.");
    }
    const auto store = SecretStoreFactory::create();
    QVERIFY(store->isPersistent());

    const auto reference = QStringLiteral("holyscreen-test/%1")
                               .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QVERIFY(store->store(reference, QStringLiteral("segredo-de-teste")));
    QCOMPARE(store->retrieve(reference), std::optional<QString>{QStringLiteral("segredo-de-teste")});
    QVERIFY(store->references().contains(reference));
    QVERIFY(store->remove(reference));
    QCOMPARE(store->retrieve(reference), std::nullopt);
}

QTEST_MAIN(SecretStoreTest)
#include "SecretStoreTest.moc"
