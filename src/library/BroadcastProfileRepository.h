#pragma once

#include "screens/BroadcastProfile.h"

#include <QString>
#include <QVector>

#include <optional>

namespace churchpresenter {

//! Persistência dos perfis de saída de transmissão em `output_broadcast_profiles`.
class BroadcastProfileRepository final {
public:
    explicit BroadcastProfileRepository(QString databasePath);
    ~BroadcastProfileRepository();

    BroadcastProfileRepository(const BroadcastProfileRepository &) = delete;
    BroadcastProfileRepository &operator=(const BroadcastProfileRepository &) = delete;

    bool open();
    bool save(const BroadcastProfile &profile);
    [[nodiscard]] std::optional<BroadcastProfile> find(const QString &screenFingerprint) const;
    //! Perfil salvo ou os padrões da saída, sem gravar nada.
    [[nodiscard]] BroadcastProfile findOrDefault(const QString &screenFingerprint) const;
    [[nodiscard]] QVector<BroadcastProfile> all() const;
    bool remove(const QString &screenFingerprint);

private:
    QString m_databasePath;
    QString m_connectionName;
};

} // namespace churchpresenter
