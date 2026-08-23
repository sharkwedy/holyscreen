import QtQuick

Item {
    id: root

    required property var controller
    property bool isBlackout: false
    property string textOverride: ""
    visible: true
    opacity: root.controller.textVisible && !isBlackout ? 1 : 0

    Behavior on opacity { NumberAnimation { duration: root.controller.activeTheme.transition === "fade" ? 220 : 0; easing.type: Easing.InOutQuad } }

    Rectangle {
        anchors.fill: parent
        visible: root.controller.activeTheme.backgroundType === 0
        color: root.controller.activeTheme.backgroundColor || "#000000"
    }
    Image {
        anchors.fill: parent
        visible: root.controller.activeTheme.backgroundType === 2 && source.toString().length > 0
        source: root.controller.activeTheme.backgroundImage
        fillMode: Image.PreserveAspectCrop
    }

    Text {
        anchors.fill: parent
        anchors.margins: root.controller.activeTheme.margin || 64
        text: root.textOverride.length > 0 ? root.textOverride : root.controller.currentSlideText
        color: root.controller.activeTheme.textColor || "white"
        horizontalAlignment: root.controller.activeTheme.horizontalAlignment === "left" ? Text.AlignLeft : root.controller.activeTheme.horizontalAlignment === "right" ? Text.AlignRight : Text.AlignHCenter
        verticalAlignment: root.controller.activeTheme.verticalAlignment === "top" ? Text.AlignTop : root.controller.activeTheme.verticalAlignment === "bottom" ? Text.AlignBottom : Text.AlignVCenter
        wrapMode: Text.WordWrap
        font.family: root.controller.activeTheme.fontFamily || ""
        font.weight: root.controller.activeTheme.fontWeight || 700
        font.pixelSize: root.controller.activeTheme.fontSize || 72
        minimumPixelSize: root.controller.activeTheme.minimumFontSize || 28
        lineHeight: 1 + ((root.controller.activeTheme.lineSpacing || 0) / 100)
        fontSizeMode: Text.Fit
        style: root.controller.activeTheme.outline ? Text.Outline : root.controller.activeTheme.shadow ? Text.Raised : Text.Normal
        styleColor: root.controller.activeTheme.outline ? root.controller.activeTheme.outlineColor : root.controller.activeTheme.shadowColor
    }
}
