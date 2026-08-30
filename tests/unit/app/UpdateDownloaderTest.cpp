#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "app/UpdateDownloader.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QHttpServer>
#include <QTcpServer>
#include <QTemporaryDir>

using namespace churchpresenter;

namespace {

//! Servidor local que devolve um corpo fixo. O validador de URL do downloader é
//! substituído nos testes justamente para permitir esta origem em HTTP.
class PayloadServer {
public:
    explicit PayloadServer(QByteArray payload) : m_payload(std::move(payload))
    {
        m_server.route(QStringLiteral("/pacote.bin"), [this] { return m_payload; });
        auto *socket = new QTcpServer(&m_server);
        m_listening = socket->listen(QHostAddress::LocalHost) && m_server.bind(socket);
        m_port = socket->serverPort();
    }

    [[nodiscard]] bool isListening() const { return m_listening; }
    [[nodiscard]] QUrl url() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/pacote.bin").arg(m_port));
    }

private:
    QByteArray m_payload;
    QHttpServer m_server;
    bool m_listening = false;
    quint16 m_port = 0;
};

UpdateDownloader::UrlValidator acceptLocalhost()
{
    return [](const QUrl &url) { return url.host() == QStringLiteral("127.0.0.1"); };
}

QString digestOf(const QByteArray &payload)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex());
}

} // namespace

class UpdateDownloaderTest final : public QObject {
    Q_OBJECT

private slots:
    void savesThePackageWhenTheDigestMatches();
    void refusesAPackageWhoseDigestDiffers();
    void refusesAPackageLargerThanAnnounced();
    void refusesUntrustedOrigins();
    void refusesAnUnusableDigest();
    void refusesANonHexDigestBeforeStartingTheRequest();
};

void UpdateDownloaderTest::savesThePackageWhenTheDigestMatches()
{
    const QByteArray payload(64 * 1024, 'h');
    PayloadServer server(payload);
    QVERIFY(server.isListening());
    QTemporaryDir destination;
    QVERIFY(destination.isValid());

    UpdateDownloader downloader(acceptLocalhost(), nullptr);
    QSignalSpy finished(&downloader, &UpdateDownloader::finished);
    downloader.start(server.url(), digestOf(payload), payload.size(),
                     destination.path(), QStringLiteral("HolyScreen.dmg"));
    QVERIFY(finished.wait(10000));

    const auto path = finished.first().at(0).toString();
    const auto error = finished.first().at(1).toString();
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(path, destination.filePath(QStringLiteral("HolyScreen.dmg")));

    QFile written(path);
    QVERIFY(written.open(QIODevice::ReadOnly));
    QCOMPARE(written.readAll(), payload);
    QVERIFY(!downloader.isRunning());
}

void UpdateDownloaderTest::refusesAPackageWhoseDigestDiffers()
{
    const QByteArray payload(4096, 'a');
    PayloadServer server(payload);
    QVERIFY(server.isListening());
    QTemporaryDir destination;
    QVERIFY(destination.isValid());

    UpdateDownloader downloader(acceptLocalhost(), nullptr);
    QSignalSpy finished(&downloader, &UpdateDownloader::finished);
    downloader.start(server.url(), digestOf(QByteArray(4096, 'b')), payload.size(),
                     destination.path(), QStringLiteral("HolyScreen.dmg"));
    QVERIFY(finished.wait(10000));

    QVERIFY(finished.first().at(0).toString().isEmpty());
    QVERIFY(finished.first().at(1).toString().contains(QStringLiteral("SHA-256")));
    // Um pacote que não confere não pode sobrar no disco do operador.
    QVERIFY(!QFile::exists(destination.filePath(QStringLiteral("HolyScreen.dmg"))));
}

void UpdateDownloaderTest::refusesAPackageLargerThanAnnounced()
{
    const QByteArray payload(8192, 'c');
    PayloadServer server(payload);
    QVERIFY(server.isListening());
    QTemporaryDir destination;
    QVERIFY(destination.isValid());

    UpdateDownloader downloader(acceptLocalhost(), nullptr);
    QSignalSpy finished(&downloader, &UpdateDownloader::finished);
    downloader.start(server.url(), digestOf(payload), 1024,
                     destination.path(), QStringLiteral("HolyScreen.dmg"));
    QVERIFY(finished.wait(10000));

    QVERIFY(finished.first().at(0).toString().isEmpty());
    QVERIFY(!finished.first().at(1).toString().isEmpty());
    QVERIFY(!QFile::exists(destination.filePath(QStringLiteral("HolyScreen.dmg"))));
}

void UpdateDownloaderTest::refusesUntrustedOrigins()
{
    QTemporaryDir destination;
    QVERIFY(destination.isValid());

    // O validador padrão exige HTTPS no GitHub.
    UpdateDownloader downloader;
    QSignalSpy finished(&downloader, &UpdateDownloader::finished);
    downloader.start(QUrl(QStringLiteral("http://exemplo.invalido/pacote.dmg")),
                     digestOf(QByteArrayLiteral("x")), 1,
                     destination.path(), QStringLiteral("HolyScreen.dmg"));
    QCOMPARE(finished.size(), 1);
    QVERIFY(finished.first().at(1).toString().contains(QStringLiteral("confiável")));
    QVERIFY(!downloader.isRunning());
}

void UpdateDownloaderTest::refusesAnUnusableDigest()
{
    QTemporaryDir destination;
    QVERIFY(destination.isValid());

    UpdateDownloader downloader(acceptLocalhost(), nullptr);
    QSignalSpy finished(&downloader, &UpdateDownloader::finished);
    downloader.start(QUrl(QStringLiteral("http://127.0.0.1:1/pacote.bin")),
                     QStringLiteral("curto"), 10,
                     destination.path(), QStringLiteral("HolyScreen.dmg"));
    QCOMPARE(finished.size(), 1);
    QVERIFY(!finished.first().at(1).toString().isEmpty());
}

void UpdateDownloaderTest::refusesANonHexDigestBeforeStartingTheRequest()
{
    QTemporaryDir destination;
    QVERIFY(destination.isValid());

    UpdateDownloader downloader(acceptLocalhost(), nullptr);
    QSignalSpy finished(&downloader, &UpdateDownloader::finished);
    downloader.start(QUrl(QStringLiteral("http://127.0.0.1:1/pacote.bin")),
                     QString(64, QLatin1Char('z')), 10,
                     destination.path(), QStringLiteral("HolyScreen.dmg"));

    QCOMPARE(finished.size(), 1);
    QVERIFY(finished.first().at(0).toString().isEmpty());
    QVERIFY(!finished.first().at(1).toString().isEmpty());
    QVERIFY(!downloader.isRunning());
}

QTEST_MAIN(UpdateDownloaderTest)

#include "UpdateDownloaderTest.moc"
