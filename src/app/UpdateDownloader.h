#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>

#include <functional>
#include <memory>

QT_FORWARD_DECLARE_CLASS(QNetworkReply)

namespace churchpresenter {

//! Baixa o pacote de uma release e só entrega o arquivo depois de conferir o
//! SHA-256 publicado pelo GitHub.
//!
//! O arquivo é escrito por streaming, então uma atualização de centenas de
//! megabytes não carrega tudo em memória, e o download é abortado assim que
//! passa do tamanho anunciado. Um digest divergente descarta o arquivo: o
//! operador nunca recebe um instalador que não corresponde ao que a release
//! publicou.
class UpdateDownloader final : public QObject {
    Q_OBJECT

public:
    //! Decide se um endereço pode ser baixado. O padrão exige HTTPS no GitHub,
    //! a mesma regra do manifesto; os testes injetam a própria validação para
    //! poderem usar um servidor local.
    using UrlValidator = std::function<bool(const QUrl &)>;

    explicit UpdateDownloader(QObject *parent = nullptr);
    UpdateDownloader(UrlValidator validator, QObject *parent);
    // Definido no .cpp, onde Transfer é completo.
    ~UpdateDownloader() override;

    //! Inicia o download de \a url em \a destinationDirectory com o nome
    //! \a fileName. Um download já em andamento é recusado.
    void start(const QUrl &url, const QString &expectedSha256, qint64 expectedSize,
               const QString &destinationDirectory, const QString &fileName);
    void cancel();

    [[nodiscard]] bool isRunning() const;

signals:
    void progress(qint64 received, qint64 total);
    //! \a path fica vazio quando \a error descreve a falha.
    void finished(const QString &path, const QString &error);

private:
    void fail(const QString &error);

    UrlValidator m_validator;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    struct Transfer;
    std::unique_ptr<Transfer> m_transfer;
};

} // namespace churchpresenter
