#include <QtTest/QTest>

#include "remote/RemoteQrCode.h"

using namespace churchpresenter;

class RemoteQrCodeTest final : public QObject {
    Q_OBJECT

private slots:
    void producesSelfContainedSvgDataUrl();
    void rejectsEmptyContent();
};

void RemoteQrCodeTest::producesSelfContainedSvgDataUrl()
{
    const auto result = RemoteQrCode::svgDataUrl(
        QStringLiteral("http://192.168.1.20:43120"));

    QVERIFY(result.startsWith(QStringLiteral("data:image/svg+xml;base64,")));
    const auto svg = QByteArray::fromBase64(result.section(QLatin1Char(','), 1).toLatin1());
    QVERIFY(svg.startsWith("<svg"));
    QVERIFY(svg.contains("<path"));
    QVERIFY(svg.contains("viewBox=\"0 0 "));
}

void RemoteQrCodeTest::rejectsEmptyContent()
{
    QVERIFY(RemoteQrCode::svgDataUrl(QString{}).isEmpty());
}

QTEST_APPLESS_MAIN(RemoteQrCodeTest)
#include "RemoteQrCodeTest.moc"
