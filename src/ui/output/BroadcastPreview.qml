pragma ComponentBehavior: Bound

import QtQuick

// Prévia da saída de transmissão no operador. O xadrez atrás da composição
// mostra o que ficará transparente na captura.
Rectangle {
    id: root

    required property var controller
    property var profile: ({})

    color: "#0e1726"
    clip: true

    Grid {
        id: checkerboard
        anchors.fill: parent
        columns: Math.max(1, Math.ceil(root.width / 16))
        rows: Math.max(1, Math.ceil(root.height / 16))
        Repeater {
            model: checkerboard.columns * checkerboard.rows
            delegate: Rectangle {
                id: checkerCell
                required property int index
                width: 16
                height: 16
                color: (checkerCell.index % checkerboard.columns
                        + Math.floor(checkerCell.index / checkerboard.columns)) % 2 === 0
                       ? "#1b2740" : "#131c2f"
            }
        }
    }

    BroadcastView {
        anchors.fill: parent
        controller: root.controller
        profile: root.profile
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: "#38506f"
        border.width: 1
        radius: 2
    }
}
