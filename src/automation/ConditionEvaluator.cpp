#include "automation/ConditionEvaluator.h"

#include <QVariantList>

#include <algorithm>

namespace churchpresenter {
namespace {

QVariant lookup(const QVariantMap &source, const QString &path)
{
    const auto parts = path.split(QLatin1Char('.'));
    QVariant current = source;
    for (const auto &part : parts) {
        if (current.typeId() != QMetaType::QVariantMap) return {};
        current = current.toMap().value(part);
        if (!current.isValid()) return {};
    }
    return current;
}

bool numeric(const QVariant &value, double *number)
{
    bool ok = false;
    const auto converted = value.toDouble(&ok);
    if (ok) *number = converted;
    return ok;
}

QTime timeFrom(const QVariant &value)
{
    if (value.typeId() == QMetaType::QTime) return value.toTime();
    return QTime::fromString(value.toString().trimmed(), QStringLiteral("HH:mm"));
}

} // namespace

QVariant ConditionEvaluator::valueOf(const QString &field, const Context &context)
{
    const auto trimmed = field.trimmed();
    if (trimmed.startsWith(QStringLiteral("event."))) {
        return lookup(context.event, trimmed.mid(6));
    }
    if (trimmed.startsWith(QStringLiteral("state."))) {
        return lookup(context.state, trimmed.mid(6));
    }
    if (trimmed == QStringLiteral("time")) return context.localTime;
    const auto fromEvent = lookup(context.event, trimmed);
    return fromEvent.isValid() ? fromEvent : lookup(context.state, trimmed);
}

bool ConditionEvaluator::matches(const Condition &condition, const Context &context)
{
    const auto actual = valueOf(condition.field, context);
    const auto &operation = condition.operation;

    if (operation == QStringLiteral("isEmpty")) {
        return !actual.isValid() || actual.toString().trimmed().isEmpty();
    }
    if (operation == QStringLiteral("isNotEmpty")) {
        return actual.isValid() && !actual.toString().trimmed().isEmpty();
    }
    if (operation == QStringLiteral("equals")) {
        return actual.toString().compare(condition.expected.toString(), Qt::CaseInsensitive) == 0;
    }
    if (operation == QStringLiteral("notEquals")) {
        return actual.toString().compare(condition.expected.toString(), Qt::CaseInsensitive) != 0;
    }
    if (operation == QStringLiteral("contains")) {
        return actual.toString().contains(condition.expected.toString(), Qt::CaseInsensitive);
    }
    if (operation == QStringLiteral("notContains")) {
        return !actual.toString().contains(condition.expected.toString(), Qt::CaseInsensitive);
    }
    if (operation == QStringLiteral("greaterThan") || operation == QStringLiteral("lessThan")) {
        double left = 0.0;
        double right = 0.0;
        if (!numeric(actual, &left) || !numeric(condition.expected, &right)) return false;
        return operation == QStringLiteral("greaterThan") ? left > right : left < right;
    }
    if (operation == QStringLiteral("between")) {
        const auto bounds = condition.expected.toList();
        double value = 0.0;
        double low = 0.0;
        double high = 0.0;
        if (bounds.size() != 2 || !numeric(actual, &value) || !numeric(bounds[0], &low)
            || !numeric(bounds[1], &high)) {
            return false;
        }
        if (low > high) std::swap(low, high);
        return value >= low && value <= high;
    }
    if (operation == QStringLiteral("timeBetween")) {
        const auto bounds = condition.expected.toList();
        if (bounds.size() != 2) return false;
        const auto start = timeFrom(bounds[0]);
        const auto end = timeFrom(bounds[1]);
        const auto moment = condition.field.trimmed().isEmpty()
            ? context.localTime : timeFrom(actual.isValid() ? actual : QVariant(context.localTime));
        if (!start.isValid() || !end.isValid() || !moment.isValid()) return false;
        // Períodos que cruzam a meia-noite continuam válidos.
        if (start <= end) return moment >= start && moment <= end;
        return moment >= start || moment <= end;
    }
    return false;
}

bool ConditionEvaluator::matches(const QList<Condition> &conditions, ConditionGroup group,
                                 const Context &context)
{
    if (conditions.isEmpty()) return true;
    if (group == ConditionGroup::Any) {
        return std::any_of(conditions.cbegin(), conditions.cend(),
                           [&context](const Condition &condition) {
            return matches(condition, context);
        });
    }
    return std::all_of(conditions.cbegin(), conditions.cend(),
                       [&context](const Condition &condition) {
        return matches(condition, context);
    });
}

} // namespace churchpresenter
