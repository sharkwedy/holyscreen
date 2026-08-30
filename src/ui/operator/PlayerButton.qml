import QtQuick
import QtQuick.Controls

Button {
    id: playerButton

    implicitWidth: 48
    implicitHeight: 42
    flat: true
    font.pixelSize: UiScale.px(22)
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
        // O clique do mouse mantém activeFocus no botão. visualFocus só fica
        // ativo para navegação por teclado, evitando contornos em controles
        // inativos sem perder a indicação acessível de foco.
        border.color: playerButton.visualFocus || playerButton.highlighted
                      ? "#9fb3ff" : "transparent"
        border.width: playerButton.visualFocus ? 2 : 1
    }
}
