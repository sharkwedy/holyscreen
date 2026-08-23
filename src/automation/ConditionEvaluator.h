#pragma once

#include "automation/AutomationTypes.h"

#include <QTime>

namespace churchpresenter {

//! Avalia as condições de uma automação contra o contexto do gatilho. Não há
//! linguagem de script: apenas comparações declaradas.
class ConditionEvaluator final {
public:
    struct Context {
        //! Campos do evento que disparou a automação.
        QVariantMap event;
        //! Estado corrente exposto pela aplicação (evento ativo, mídia, saídas).
        QVariantMap state;
        //! Hora local usada por `timeBetween`.
        QTime localTime;
    };

    [[nodiscard]] static bool matches(const Condition &condition, const Context &context);
    [[nodiscard]] static bool matches(const QList<Condition> &conditions, ConditionGroup group,
                                      const Context &context);
    //! Resolve `event.x`, `state.y` ou um campo simples do evento.
    [[nodiscard]] static QVariant valueOf(const QString &field, const Context &context);
};

} // namespace churchpresenter
