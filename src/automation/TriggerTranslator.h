#pragma once

#include "automation/AutomationTypes.h"
#include "core/EventTypes.h"

#include <optional>

namespace churchpresenter {

//! Traduz fatos publicados no EventBus para gatilhos de automação. Mantém a
//! automação desacoplada dos nomes internos de comando.
class TriggerTranslator final {
public:
    struct Match {
        QString triggerType;
        QVariantMap payload;
        QString correlationId;
    };

    [[nodiscard]] static std::optional<Match> translate(const DomainEvent &event);
};

} // namespace churchpresenter
