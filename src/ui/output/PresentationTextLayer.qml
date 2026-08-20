// qmllint disable unqualified
import QtQuick

Item {
    id: root
    property bool isBlackout: false
    property string textOverride: ""
    visible: true
    opacity: presentationController.textVisible && !isBlackout ? 1 : 0

    Behavior on opacity { NumberAnimation { duration: presentationController.activeTheme.transition === "fade" ? 220 : 0; easing.type: Easing.InOutQuad } }

    Rectangle {
        anchors.fill: parent
        visible: presentationController.activeTheme.backgroundType === 0
        color: presentationController.activeTheme.backgroundColor || "#000000"
    }
    Image {
        anchors.fill: parent
        visible: presentationController.activeTheme.backgroundType === 2 && source.toString().length > 0
        source: presentationController.activeTheme.backgroundImage
        fillMode: Image.PreserveAspectCrop
    }

    Text {
        anchors.fill: parent
        anchors.margins: presentationController.activeTheme.margin || 64
        text: root.textOverride.length > 0 ? root.textOverride : presentationController.currentSlideText
        color: presentationController.activeTheme.textColor || "white"
        horizontalAlignment: presentationController.activeTheme.horizontalAlignment === "left" ? Text.AlignLeft : presentationController.activeTheme.horizontalAlignment === "right" ? Text.AlignRight : Text.AlignHCenter
        verticalAlignment: presentationController.activeTheme.verticalAlignment === "top" ? Text.AlignTop : presentationController.activeTheme.verticalAlignment === "bottom" ? Text.AlignBottom : Text.AlignVCenter
        wrapMode: Text.WordWrap
        font.family: presentationController.activeTheme.fontFamily || ""
        font.weight: presentationController.activeTheme.fontWeight || 700
        font.pixelSize: presentationController.activeTheme.fontSize || 72
        minimumPixelSize: presentationController.activeTheme.minimumFontSize || 28
        lineHeight: 1 + ((presentationController.activeTheme.lineSpacing || 0) / 100)
        fontSizeMode: Text.Fit
        style: presentationController.activeTheme.outline ? Text.Outline : presentationController.activeTheme.shadow ? Text.Raised : Text.Normal
        styleColor: presentationController.activeTheme.outline ? presentationController.activeTheme.outlineColor : presentationController.activeTheme.shadowColor
    }
}
