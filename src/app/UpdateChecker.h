#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

namespace churchpresenter {

class UpdateChecker final : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);

    void check(const QUrl &endpoint, const QString &currentVersion);

signals:
    void completed(QString latestVersion, QUrl downloadUrl, bool available, QString error);

private:
    QNetworkAccessManager m_network;
};

} // namespace churchpresenter
