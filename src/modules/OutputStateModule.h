#pragma once

#include "screens/BroadcastProfile.h"
#include "screens/OutputManager.h"
#include "screens/OutputRole.h"
#include "screens/OutputRouting.h"

#include <QHash>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace churchpresenter {

//! Owns the persistent state of the external outputs: which screens are
//! active, their role, name, Bible translation and media flag, plus the
//! serialization used by the settings repository and the view models consumed
//! by QML. The application controller keeps only the orchestration.
class OutputStateModule final {
public:
    //! Field separator of the persisted `outputs/items` entries.
    static constexpr QChar FieldSeparator = QChar(u'\x1f');

    [[nodiscard]] OutputManager &manager();
    [[nodiscard]] const OutputManager &manager() const;
    [[nodiscard]] const QVector<OutputDescriptor> &activeOutputs() const;

    //! Restores outputs persisted by this or by any previous version. Entries
    //! written before a role existed load as Audience; an unknown role never
    //! discards the output, it only falls back to Audience.
    void restore(const QStringList &serialized);
    [[nodiscard]] QStringList serialize() const;

    void applyScreens(const QVector<ScreenDescriptor> &screens);
    [[nodiscard]] EnableOutputResult enable(const ScreenDescriptor &screen);
    void disable(const QString &screenFingerprint);
    qsizetype enableAllAudienceScreens(const QVector<ScreenDescriptor> &screens);
    bool setRole(const QString &screenFingerprint, const QString &role);
    bool setBibleTranslation(const QString &screenFingerprint, const QString &translationId);
    bool setMediaEnabled(const QString &screenFingerprint, bool enabled);
    bool setDisplayName(const QString &screenFingerprint, const QString &displayName);

    //! Substitui o cache de perfis de transmissão carregado do banco.
    void setBroadcastProfiles(const QVector<BroadcastProfile> &profiles);
    //! Perfil salvo da saída ou os padrões, sempre com o fingerprint preenchido.
    [[nodiscard]] BroadcastProfile broadcastProfile(const QString &screenFingerprint) const;
    //! Aplica uma alteração parcial e devolve o perfil resultante para que a
    //! camada de aplicação o persista.
    [[nodiscard]] BroadcastProfile mergeBroadcastProfile(const QString &screenFingerprint,
                                                         const QVariantMap &changes);

    //! View model of every detected screen, including the output settings when
    //! the screen is active.
    [[nodiscard]] QVariantList describeScreens(const QVector<ScreenDescriptor> &screens) const;
    //! View model of the windows that must exist for the current screens.
    [[nodiscard]] QVariantList describeOutputWindows(
        const QVector<ScreenDescriptor> &screens) const;

private:
    OutputManager m_outputs;
    QHash<QString, BroadcastProfile> m_broadcastProfiles;
};

} // namespace churchpresenter
