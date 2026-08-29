#pragma once

#include <QObject>
#include <QString>

#include <functional>

namespace churchpresenter {

struct UpdateInstallRequest {
    QString installerPath;
    QString expectedSha256;
    qint64 expectedSize = 0;
};

//! Revalida e inicia o instalador de uma atualização já baixada.
//!
//! A verificação é repetida imediatamente antes da execução para impedir que
//! um arquivo trocado depois do download seja iniciado. A instalação automática
//! fica restrita ao instalador NSIS do Windows; os demais pacotes continuam
//! disponíveis pelo fluxo manual do sistema operacional.
class UpdateInstaller final : public QObject {
    Q_OBJECT

public:
    using Launcher = std::function<bool(const UpdateInstallRequest &)>;

    explicit UpdateInstaller(QObject *parent = nullptr);
    UpdateInstaller(Launcher launcher, bool supported, QObject *parent = nullptr);

    [[nodiscard]] bool canInstall(const QString &path) const;
    [[nodiscard]] bool install(const QString &path, const QString &expectedSha256,
                               qint64 expectedSize, QString *error = nullptr) const;

private:
    Launcher m_launcher;
    bool m_supported = false;
};

} // namespace churchpresenter
