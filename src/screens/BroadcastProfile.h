#pragma once

#include <QMarginsF>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include <optional>

namespace churchpresenter {

enum class BroadcastBackgroundMode {
    Transparent,
    Chroma,
};

enum class BroadcastAspectPreset {
    Landscape, //!< 16:9
    Portrait,  //!< 9:16
};

//! Configuração persistente de uma saída de transmissão. As margens da zona
//! segura são percentuais da caixa de composição, nunca pixels, para não
//! assumir resolução física.
struct BroadcastProfile {
    QString screenFingerprint;
    BroadcastBackgroundMode backgroundMode = BroadcastBackgroundMode::Chroma;
    QString chromaColor = QStringLiteral("#00b140");
    QMarginsF safeArea{5.0, 5.0, 5.0, 5.0};
    BroadcastAspectPreset aspectPreset = BroadcastAspectPreset::Landscape;
    bool showClock = false;
    bool showLowerThird = true;
    bool showAlerts = true;
    bool showAudienceMessage = true;
};

[[nodiscard]] QString broadcastBackgroundModeName(BroadcastBackgroundMode mode);
[[nodiscard]] std::optional<BroadcastBackgroundMode> broadcastBackgroundModeFromName(
    const QString &name);
[[nodiscard]] QStringList broadcastBackgroundModeNames();

[[nodiscard]] QString broadcastAspectPresetName(BroadcastAspectPreset preset);
[[nodiscard]] std::optional<BroadcastAspectPreset> broadcastAspectPresetFromName(
    const QString &name);
[[nodiscard]] QStringList broadcastAspectPresetNames();
//! Proporção largura/altura do preset, usada para calcular a caixa de
//! composição dentro da janela real.
[[nodiscard]] double broadcastAspectRatio(BroadcastAspectPreset preset);

//! Valida e normaliza um perfil: cor de chroma legível, margens entre 0 e 45%
//! por lado e soma inferior a 90% em cada eixo.
[[nodiscard]] BroadcastProfile normalizedBroadcastProfile(const BroadcastProfile &profile);
[[nodiscard]] bool isValidChromaColor(const QString &color);

[[nodiscard]] QVariantMap broadcastProfileToMap(const BroadcastProfile &profile);
//! Lê um perfil de um mapa parcial: campos ausentes ou inválidos mantêm o
//! valor de \a fallback, e a leitura nunca falha silenciosamente para outro
//! modo de fundo.
[[nodiscard]] BroadcastProfile broadcastProfileFromMap(const QVariantMap &map,
                                                       const BroadcastProfile &fallback = {});

[[nodiscard]] bool operator==(const BroadcastProfile &left, const BroadcastProfile &right);

} // namespace churchpresenter
