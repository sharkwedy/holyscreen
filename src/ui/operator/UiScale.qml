pragma Singleton

import QtQml

QtObject {
    property real factor: 1.0

    function px(value) {
        return Math.max(1, Math.round(value * factor))
    }
}
