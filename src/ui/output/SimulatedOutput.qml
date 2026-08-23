import QtQuick
import QtQuick.Controls
import QtMultimedia

Rectangle {
    id: root

    required property var controller
    property string outputLabel: "SIMULAÇÃO"
    property color wallpaper: "#000000"
    property url wallpaperSource
    property string wallpaperFit: "cover"
    property bool showClock: true
    property string clockText: "00:00"
    property string clockPosition: "bottomRight"
    property string clockFamily: ""
    property int clockFontSize: 64
    property color clockColor: "white"
    property bool clockFontBold: true
    property bool clockFontItalic: false
    property color clockBackgroundColor: "black"
    property real clockLineHeight: 1.0
    property int clockCornerRadius: 12
    property real clockTextOpacity: 1.0
    property real clockBackgroundOpacity: 0.5
    property int clockMarginHorizontal: 0
    property int clockMarginVertical: 0
    property string clockEffect: "outline"
    property bool isBlackout: false
    property bool identifyVisible: false
    property int identifier: 1

    color: isBlackout ? "#000000" : wallpaper
    radius: 10
    border.color: "#38506f"
    border.width: 1
    clip: true

    Image {
        anchors.fill: parent
        visible: !root.isBlackout && source.toString().length > 0
        source: root.wallpaperSource
        asynchronous: true
        fillMode: root.wallpaperFit === "contain" ? Image.PreserveAspectFit
                  : root.wallpaperFit === "stretch" ? Image.Stretch
                  : root.wallpaperFit === "center" ? Image.Pad
                  : Image.PreserveAspectCrop
    }

    PresentationImageLayer {
        anchors.fill: parent
        controller: root.controller
        isBlackout: root.isBlackout
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        visible: root.controller.videoVisible && !root.isBlackout
        fillMode: VideoOutput.PreserveAspectFit
        Component.onCompleted: root.controller.registerVideoSink(videoSink)
        Component.onDestruction: root.controller.unregisterVideoSink(videoSink)
    }

    PresentationTextLayer {
        anchors.fill: parent
        controller: root.controller
        isBlackout: root.isBlackout
    }

    Label {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 12
        text: root.outputLabel
        color: "#dbeafe"
        font.bold: true
        font.pixelSize: 11
        opacity: root.isBlackout ? 0.25 : 0.85
    }

    Item {
        id: clockPanel
        visible: root.showClock && !root.isBlackout
        width: simulatedClockText.implicitWidth + 12
        height: simulatedClockText.implicitHeight + 8
        x: root.clockPosition.endsWith("Left")
           ? 18 + parent.width * root.clockMarginHorizontal / 100
           : root.clockPosition.endsWith("Right")
             ? parent.width - width - 18 + parent.width * root.clockMarginHorizontal / 100
             : (parent.width - width) / 2 + parent.width * root.clockMarginHorizontal / 100
        y: root.clockPosition.startsWith("top")
           ? 18 + parent.height * root.clockMarginVertical / 100
           : root.clockPosition.startsWith("bottom")
             ? parent.height - height - 18 + parent.height * root.clockMarginVertical / 100
             : (parent.height - height) / 2 + parent.height * root.clockMarginVertical / 100
        Rectangle {
            anchors.fill: parent
            color: root.clockBackgroundColor
            opacity: root.clockBackgroundOpacity
            radius: root.clockCornerRadius * Math.min(root.width / 1920, root.height / 1080)
        }
        Text {
            id: simulatedClockText
            anchors.centerIn: parent
            text: root.clockText
            color: root.clockColor
            opacity: root.clockTextOpacity
            font.family: root.clockFamily
            font.pixelSize: Math.max(20, root.clockFontSize * Math.min(root.width / 1920,
                                                                       root.height / 1080))
            font.bold: root.clockFontBold
            font.italic: root.clockFontItalic
            lineHeight: root.clockLineHeight
            lineHeightMode: Text.ProportionalHeight
            style: root.clockEffect === "outline" ? Text.Outline
                 : root.clockEffect === "raised" ? Text.Raised
                 : root.clockEffect === "sunken" ? Text.Sunken : Text.Normal
            styleColor: "#b0000000"
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.identifyVisible
        color: "#0f172a"
        opacity: 0.9
        Label {
            anchors.centerIn: parent
            text: root.identifier
            color: "white"
            font.bold: true
            font.pixelSize: Math.min(parent.width, parent.height) * 0.55
        }
    }
}
