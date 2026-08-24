import QtQuick
import QtQuick.Controls

Button {
    id: playerButton

    implicitWidth: 48
    implicitHeight: 42
    flat: true
    font.pixelSize: 22
    font.bold: true

    contentItem: Label {
        text: playerButton.text
        color: playerButton.enabled ? "#f2f4f5" : "#626a70"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font: playerButton.font
    }

    background: Rectangle {
        radius: 6
        color: playerButton.down ? "#526173"
              : playerButton.hovered ? "#3c4650" : "transparent"
        border.color: playerButton.highlighted ? "#9fb3ff" : "transparent"
    }
}
