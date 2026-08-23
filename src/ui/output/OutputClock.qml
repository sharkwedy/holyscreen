import QtQuick

// Relógio das saídas de público e transmissão. Recebe o controlador como
// propriedade para que o componente permaneça um renderizador puro.
Item {
    id: root

    property var controller
    property bool isBlackout: false

    visible: root.controller.clockVisible && !root.isBlackout
    width: clockLabel.implicitWidth + 28
    height: clockLabel.implicitHeight + 18
    x: root.controller.clockPosition.endsWith("Left")
       ? 40 + parent.width * root.controller.clockMarginHorizontal / 100
       : root.controller.clockPosition.endsWith("Right")
         ? parent.width - width - 40
           + parent.width * root.controller.clockMarginHorizontal / 100
         : (parent.width - width) / 2
           + parent.width * root.controller.clockMarginHorizontal / 100
    y: root.controller.clockPosition.startsWith("top")
       ? 40 + parent.height * root.controller.clockMarginVertical / 100
       : root.controller.clockPosition.startsWith("bottom")
         ? parent.height - height - 40
           + parent.height * root.controller.clockMarginVertical / 100
         : (parent.height - height) / 2
           + parent.height * root.controller.clockMarginVertical / 100

    Rectangle {
        anchors.fill: parent
        color: root.controller.clockBackgroundColor
        opacity: root.controller.clockBackgroundOpacity
        radius: root.controller.clockCornerRadius
    }

    Text {
        id: clockLabel
        anchors.centerIn: parent
        text: root.controller.clockText
        color: root.controller.clockColor
        opacity: root.controller.clockTextOpacity
        font.bold: root.controller.clockFontBold
        font.italic: root.controller.clockFontItalic
        font.family: root.controller.clockFontFamily
        font.pixelSize: root.controller.clockFontSize
        lineHeight: root.controller.clockLineHeight
        lineHeightMode: Text.ProportionalHeight
        style: root.controller.clockEffect === "outline" ? Text.Outline
             : root.controller.clockEffect === "raised" ? Text.Raised
             : root.controller.clockEffect === "sunken" ? Text.Sunken
             : Text.Normal
        styleColor: "#b0000000"
    }
}
