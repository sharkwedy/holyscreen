#include "remote/RemoteQrCode.h"

#include <qrcodegen.hpp>

#include <QByteArray>

#include <exception>

namespace churchpresenter {

QString RemoteQrCode::svgDataUrl(const QString &text)
{
    if (text.isEmpty()) return {};

    try {
        const auto utf8 = text.toUtf8();
        const auto qr = qrcodegen::QrCode::encodeText(
            utf8.constData(), qrcodegen::QrCode::Ecc::MEDIUM);
        constexpr int border = 4;
        const auto extent = qr.getSize() + border * 2;

        QByteArray path;
        for (int y = 0; y < qr.getSize(); ++y) {
            for (int x = 0; x < qr.getSize(); ++x) {
                if (!qr.getModule(x, y)) continue;
                path += "M" + QByteArray::number(x + border) + " "
                    + QByteArray::number(y + border) + "h1v1h-1z";
            }
        }

        QByteArray svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 ";
        svg += QByteArray::number(extent) + " " + QByteArray::number(extent)
            + "\" shape-rendering=\"crispEdges\"><rect width=\"100%\" height=\"100%\" fill=\"white\"/><path d=\""
            + path + "\" fill=\"black\"/></svg>";
        return QStringLiteral("data:image/svg+xml;base64,")
            + QString::fromLatin1(svg.toBase64());
    } catch (const std::exception &) {
        return {};
    }
}

} // namespace churchpresenter
