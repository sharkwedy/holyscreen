#pragma once

#include "integrations/ports/ITransports.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QList>

namespace churchpresenter {

//! Transporte HTTP sobre QNetworkAccessManager, com TLS validado por padrão,
//! timeout, limite de resposta e redirecionamento seguro.
class QtHttpTransport final : public QObject, public IHttpTransport {
    Q_OBJECT

public:
    static constexpr int MaximumRedirects = 3;

    explicit QtHttpTransport(QObject *parent = nullptr);
    ~QtHttpTransport() override;

    void send(const HttpRequest &request, Completion completion) override;
    void cancelAll() override;

private:
    QNetworkAccessManager m_network;
    QList<QPointer<QNetworkReply>> m_pending;
};

} // namespace churchpresenter
