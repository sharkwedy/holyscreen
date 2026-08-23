#include "integrations/adapters/OscIntegrationAdapter.h"
#include "integrations/adapters/OscMessage.h"
#include "integrations/transports/QtOscTransport.h"

#include <QNetworkDatagram>
#include <QTest>
#include <QUdpSocket>

using namespace churchpresenter;

namespace {

class FakeOscTransport final : public IOscTransport {
public:
    bool send(const QString &host, quint16 port, const QByteArray &datagram) override
    {
        lastHost = host;
        lastPort = port;
        lastDatagram = datagram;
        ++sent;
        return accepted;
    }
    QString lastError() const override { return QStringLiteral("porta ocupada"); }

    QString lastHost;
    quint16 lastPort = 0;
    QByteArray lastDatagram;
    int sent = 0;
    bool accepted = true;
};

IntegrationDefinition oscDefinition()
{
    return IntegrationDefinition{
        .id = QStringLiteral("luzes"),
        .name = QStringLiteral("Mesa de luz"),
        .type = IntegrationType::Osc,
        .enabled = true,
        .configuration = {{QStringLiteral("host"), QStringLiteral("127.0.0.1")},
                          {QStringLiteral("port"), 9000},
                          {QStringLiteral("address"), QStringLiteral("/cena/1")}},
        .timeoutMs = 1000,
    };
}

} // namespace

class OscIntegrationAdapterTest final : public QObject {
    Q_OBJECT

private slots:
    void encodesKnownBytesForEachArgumentType();
    void rejectsInvalidAddressesAndArguments();
    void enforcesTheDatagramLimit();
    void validatesHostPortAndPath();
    void sendsTheEncodedDatagramWithoutOpeningAListener();
    void deliversARealDatagramToALocalListener();
};

void OscIntegrationAdapterTest::encodesKnownBytesForEachArgumentType()
{
    // Mensagem OSC clássica: "/oscillator/4/frequency" com um float 440.0.
    const auto frequency = OscMessage::encode(QStringLiteral("/oscillator/4/frequency"),
                                              {QVariant(440.0f)});
    QVERIFY(frequency.has_value());
    QCOMPARE(*frequency,
             QByteArray("/oscillator/4/frequency\0,f\0\0\x43\xDC\x00\x00", 32));

    const auto integer = OscMessage::encode(QStringLiteral("/foo"), {QVariant(1000)});
    QVERIFY(integer.has_value());
    QCOMPARE(*integer, QByteArray("/foo\0\0\0\0,i\0\0\x00\x00\x03\xE8", 16));

    const auto text = OscMessage::encode(QStringLiteral("/msg"), {QVariant(QStringLiteral("hi"))});
    QVERIFY(text.has_value());
    QCOMPARE(*text, QByteArray("/msg\0\0\0\0,s\0\0hi\0\0", 16));

    const auto flags = OscMessage::encode(QStringLiteral("/flag"), {QVariant(true), QVariant(false)});
    QVERIFY(flags.has_value());
    QCOMPARE(*flags, QByteArray("/flag\0\0\0,TF\0", 12));

    // Todo bloco é múltiplo de quatro bytes.
    for (const auto &datagram : {*frequency, *integer, *text, *flags}) {
        QCOMPARE(datagram.size() % 4, 0);
    }
}

void OscIntegrationAdapterTest::rejectsInvalidAddressesAndArguments()
{
    QVERIFY(!OscMessage::isValidAddress(QStringLiteral("cena/1")));
    QVERIFY(!OscMessage::isValidAddress(QStringLiteral("/cena 1")));
    QVERIFY(!OscMessage::isValidAddress(QStringLiteral("/cena*")));
    QVERIFY(OscMessage::isValidAddress(QStringLiteral("/cena/1")));

    QCOMPARE(OscMessage::encode(QStringLiteral("sem-barra"), {}), std::nullopt);
    QCOMPARE(OscMessage::encode(QStringLiteral("/x"), {QVariant(QVariantList{})}), std::nullopt);
}

void OscIntegrationAdapterTest::enforcesTheDatagramLimit()
{
    const QString huge(OscMessage::MaximumDatagramBytes, QLatin1Char('x'));
    QCOMPARE(OscMessage::encode(QStringLiteral("/x"), {QVariant(huge)}), std::nullopt);
}

void OscIntegrationAdapterTest::validatesHostPortAndPath()
{
    FakeOscTransport transport;
    OscIntegrationAdapter adapter(transport);

    QVERIFY(adapter.validate(oscDefinition()).valid);

    auto definition = oscDefinition();
    definition.configuration.insert(QStringLiteral("host"), QStringLiteral("::1"));
    QVERIFY(adapter.validate(definition).valid);

    definition = oscDefinition();
    definition.configuration.insert(QStringLiteral("host"), QStringLiteral("mesa.local"));
    QVERIFY(!adapter.validate(definition).valid);

    definition = oscDefinition();
    definition.configuration.insert(QStringLiteral("port"), 70000);
    QVERIFY(!adapter.validate(definition).valid);

    definition = oscDefinition();
    definition.configuration.insert(QStringLiteral("address"), QStringLiteral("cena"));
    QVERIFY(!adapter.validate(definition).valid);
}

void OscIntegrationAdapterTest::sendsTheEncodedDatagramWithoutOpeningAListener()
{
    FakeOscTransport transport;
    OscIntegrationAdapter adapter(transport);
    auto definition = oscDefinition();
    definition.configuration.insert(QStringLiteral("address"), QStringLiteral("/cena/{{cena}}"));

    IntegrationResult result;
    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("message.send"),
                                       .payload = {{QStringLiteral("cena"), 3},
                                                   {QStringLiteral("arguments"),
                                                    QVariantList{1, QStringLiteral("go")}}}},
                    [&result](const IntegrationResult &value) { result = value; });

    QVERIFY(result.accepted);
    QCOMPARE(transport.lastHost, QStringLiteral("127.0.0.1"));
    QCOMPARE(transport.lastPort, quint16(9000));
    QVERIFY(transport.lastDatagram.startsWith(QByteArray("/cena/3")));
    QVERIFY(transport.lastDatagram.contains(QByteArray(",is")));
    QCOMPARE(result.responseMetadata.value(QStringLiteral("address")).toString(),
             QStringLiteral("/cena/3"));

    transport.accepted = false;
    adapter.execute(definition, IntegrationRequest{.operation = QStringLiteral("message.send")},
                    [&result](const IntegrationResult &value) { result = value; });
    QVERIFY(!result.accepted);
    QCOMPARE(result.errorCode, QStringLiteral("connection_failed"));

    adapter.execute(definition, IntegrationRequest{.operation = QStringLiteral("scene.change")},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(result.errorCode, QStringLiteral("unsupported_operation"));

    IntegrationResult tested;
    adapter.test(oscDefinition(), [&tested](const IntegrationResult &value) { tested = value; });
    QVERIFY(tested.accepted);
    // O teste de conexão não envia nada ao dispositivo.
    QCOMPARE(transport.sent, 2);
}

void OscIntegrationAdapterTest::deliversARealDatagramToALocalListener()
{
    QUdpSocket listener;
    QVERIFY(listener.bind(QHostAddress::LocalHost, 0));

    QtOscTransport transport;
    OscIntegrationAdapter adapter(transport);
    auto definition = oscDefinition();
    definition.configuration.insert(QStringLiteral("port"), listener.localPort());
    definition.configuration.insert(QStringLiteral("address"), QStringLiteral("/holyscreen/slide"));

    IntegrationResult result;
    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("message.send"),
                                       .payload = {{QStringLiteral("arguments"),
                                                    QVariantList{7, true}}}},
                    [&result](const IntegrationResult &value) { result = value; });
    QVERIFY2(result.accepted, qPrintable(result.message));

    QVERIFY(listener.waitForReadyRead(2000));
    const auto datagram = listener.receiveDatagram();
    const auto data = datagram.data();
    QVERIFY(data.startsWith(QByteArray("/holyscreen/slide")));
    QVERIFY(data.contains(QByteArray(",iT")));
    QCOMPARE(data.size() % 4, 0);
    QCOMPARE(result.responseMetadata.value(QStringLiteral("bytes")).toInt(), data.size());
}

QTEST_MAIN(OscIntegrationAdapterTest)
#include "OscIntegrationAdapterTest.moc"
