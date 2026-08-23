#include "integrations/adapters/MidiIntegrationAdapter.h"
#include "integrations/transports/RtMidiTransport.h"

#include <QTest>

using namespace churchpresenter;

namespace {

class FakeMidiTransport final : public IMidiTransport {
public:
    QStringList outputPorts() const override { return ports; }

    bool openPort(const QString &portName) override
    {
        ++opened;
        if (!ports.contains(portName)) {
            error = QStringLiteral("porta ausente");
            return false;
        }
        return true;
    }

    bool send(const QString &portName, const MidiMessage &message) override
    {
        if (!ports.contains(portName)) {
            error = QStringLiteral("porta ausente");
            return false;
        }
        messages.append(message);
        return accepted;
    }

    void closeAll() override { ++closures; }
    QString lastError() const override { return error; }

    QStringList ports{QStringLiteral("Mesa MIDI")};
    QList<MidiMessage> messages;
    QString error = QStringLiteral("falha");
    int opened = 0;
    int closures = 0;
    bool accepted = true;
};

IntegrationDefinition midiDefinition(int channel = 1)
{
    return IntegrationDefinition{
        .id = QStringLiteral("mesa"),
        .name = QStringLiteral("Mesa MIDI"),
        .type = IntegrationType::Midi,
        .enabled = true,
        .configuration = {{QStringLiteral("port"), QStringLiteral("Mesa MIDI")},
                          {QStringLiteral("channel"), channel}},
        .timeoutMs = 1000,
    };
}

} // namespace

class MidiIntegrationAdapterTest final : public QObject {
    Q_OBJECT

private slots:
    void validatesPortAndChannel();
    void buildsEverySupportedMessage();
    void rejectsValuesOutsideTheMidiRange();
    void reportsAMissingPortWithoutCrashing();
    void testOnlyOpensThePortWithoutPlayingANote();
    void enumeratesRealPortsWithoutCrashing();
};

void MidiIntegrationAdapterTest::validatesPortAndChannel()
{
    FakeMidiTransport transport;
    MidiIntegrationAdapter adapter(transport);

    QVERIFY(adapter.validate(midiDefinition()).valid);
    QVERIFY(adapter.validate(midiDefinition(16)).valid);
    QVERIFY(!adapter.validate(midiDefinition(0)).valid);
    QVERIFY(!adapter.validate(midiDefinition(17)).valid);

    auto definition = midiDefinition();
    definition.configuration.insert(QStringLiteral("port"), QString{});
    QVERIFY(!adapter.validate(definition).valid);
}

void MidiIntegrationAdapterTest::buildsEverySupportedMessage()
{
    FakeMidiTransport transport;
    MidiIntegrationAdapter adapter(transport);
    const auto definition = midiDefinition(3);

    IntegrationResult result;
    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("note.on"),
                                       .payload = {{QStringLiteral("note"), 60},
                                                   {QStringLiteral("velocity"), 90}}},
                    [&result](const IntegrationResult &value) { result = value; });
    QVERIFY2(result.accepted, qPrintable(result.message));
    QCOMPARE(transport.messages.last().status, 0x90);
    QCOMPARE(transport.messages.last().channel, 3);
    QCOMPARE(transport.messages.last().data1, 60);
    QCOMPARE(transport.messages.last().data2, 90);

    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("note.off"),
                                       .payload = {{QStringLiteral("note"), 60}}},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(transport.messages.last().status, 0x80);

    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("control.change"),
                                       .payload = {{QStringLiteral("controller"), 7},
                                                   {QStringLiteral("value"), 127},
                                                   {QStringLiteral("channel"), 10}}},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(transport.messages.last().status, 0xB0);
    QCOMPARE(transport.messages.last().channel, 10);
    QCOMPARE(transport.messages.last().data1, 7);
    QCOMPARE(transport.messages.last().data2, 127);

    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("program.change"),
                                       .payload = {{QStringLiteral("program"), 12}}},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(transport.messages.last().status, 0xC0);
    QCOMPARE(transport.messages.last().data1, 12);

    adapter.execute(definition, IntegrationRequest{.operation = QStringLiteral("scene.change")},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(result.errorCode, QStringLiteral("unsupported_operation"));

    adapter.cancelAll();
    QCOMPARE(transport.closures, 1);
}

void MidiIntegrationAdapterTest::rejectsValuesOutsideTheMidiRange()
{
    FakeMidiTransport transport;
    MidiIntegrationAdapter adapter(transport);
    const auto definition = midiDefinition();
    const auto sentBefore = transport.messages.size();

    IntegrationResult result;
    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("note.on"),
                                       .payload = {{QStringLiteral("note"), 200}}},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(result.errorCode, QStringLiteral("invalid_payload"));

    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("note.on"),
                                       .payload = {{QStringLiteral("note"), 60},
                                                   {QStringLiteral("velocity"), -1}}},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(result.errorCode, QStringLiteral("invalid_payload"));

    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("note.on"),
                                       .payload = {{QStringLiteral("note"), 60},
                                                   {QStringLiteral("channel"), 99}}},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(result.errorCode, QStringLiteral("invalid_payload"));
    QCOMPARE(transport.messages.size(), sentBefore);
}

void MidiIntegrationAdapterTest::reportsAMissingPortWithoutCrashing()
{
    FakeMidiTransport transport;
    transport.ports.clear();
    MidiIntegrationAdapter adapter(transport);

    IntegrationResult result;
    adapter.execute(midiDefinition(),
                    IntegrationRequest{.operation = QStringLiteral("note.on"),
                                       .payload = {{QStringLiteral("note"), 60}}},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(result.errorCode, QStringLiteral("port_unavailable"));

    IntegrationResult tested;
    adapter.test(midiDefinition(), [&tested](const IntegrationResult &value) { tested = value; });
    QCOMPARE(tested.errorCode, QStringLiteral("port_unavailable"));

    // Hot-plug: a porta volta e a mesma definição passa a funcionar.
    transport.ports = {QStringLiteral("Mesa MIDI")};
    adapter.execute(midiDefinition(),
                    IntegrationRequest{.operation = QStringLiteral("note.on"),
                                       .payload = {{QStringLiteral("note"), 60}}},
                    [&result](const IntegrationResult &value) { result = value; });
    QVERIFY(result.accepted);
}

void MidiIntegrationAdapterTest::testOnlyOpensThePortWithoutPlayingANote()
{
    FakeMidiTransport transport;
    MidiIntegrationAdapter adapter(transport);

    IntegrationResult result;
    adapter.test(midiDefinition(), [&result](const IntegrationResult &value) { result = value; });
    QVERIFY(result.accepted);
    QCOMPARE(transport.opened, 1);
    QVERIFY(transport.messages.isEmpty());
    QCOMPARE(result.responseMetadata.value(QStringLiteral("availablePorts")).toStringList(),
             QStringList{QStringLiteral("Mesa MIDI")});
}

void MidiIntegrationAdapterTest::enumeratesRealPortsWithoutCrashing()
{
    // Sem hardware o backend precisa devolver uma lista vazia em vez de falhar.
    RtMidiTransport transport;
    const auto ports = transport.outputPorts();
    MidiIntegrationAdapter adapter(transport);
    QCOMPARE(adapter.availablePorts(), ports);

    if (ports.isEmpty()) {
        QVERIFY(!transport.openPort(QStringLiteral("Porta inexistente")));
        QVERIFY(!transport.lastError().isEmpty());
        return;
    }
    // Com portas disponíveis, abrir a primeira não pode lançar exceção.
    QVERIFY(transport.openPort(ports.first()) || !transport.lastError().isEmpty());
    transport.closeAll();
}

QTEST_MAIN(MidiIntegrationAdapterTest)
#include "MidiIntegrationAdapterTest.moc"
