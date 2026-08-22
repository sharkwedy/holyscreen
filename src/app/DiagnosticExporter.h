#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace churchpresenter {

struct DiagnosticExportRequest {
    QString destinationPath;
    QVariantMap application;
    QVariantList screens;
    QVariantMap configuration;
    QString logPath;
};

class DiagnosticExporter final {
public:
    [[nodiscard]] static QVariantMap sanitize(const QVariantMap &configuration);
    [[nodiscard]] static bool exportZip(const DiagnosticExportRequest &request,
                                        QString *error = nullptr);
};

} // namespace churchpresenter
