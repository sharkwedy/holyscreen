// qmllint disable unqualified
import QtQuick
import QtQuick.Controls
import QtMultimedia

Rectangle {
    id: root
    property string outputLabel: "SIMULAÇÃO"
    property color wallpaper: "#000000"
    property url wallpaperSource
    property string wallpaperFit: "cover"
    property bool showClock: true
    property string clockText: "00:00"
    property string clockPosition: "bottomRight"
    property string clockFamily: ""
    property color clockColor: "white"
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
        isBlackout: root.isBlackout
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        visible: presentationController.videoVisible && !root.isBlackout
        fillMode: VideoOutput.PreserveAspectFit
        Component.onCompleted: presentationController.registerVideoSink(videoSink)
        Component.onDestruction: presentationController.unregisterVideoSink(videoSink)
    }

    PresentationTextLayer {
        anchors.fill: parent
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

    Text {
        text: root.clockText
        color: root.clockColor
        font.family: root.clockFamily
        font.pixelSize: Math.max(20, Math.min(root.width, root.height) * 0.15)
        font.bold: true
        visible: root.showClock && !root.isBlackout
        anchors.margins: 18
        anchors.right: root.clockPosition.endsWith("Right") ? parent.right : undefined
        anchors.left: root.clockPosition.endsWith("Left") ? parent.left : undefined
        anchors.bottom: root.clockPosition.startsWith("bottom") ? parent.bottom : undefined
        anchors.top: root.clockPosition.startsWith("top") ? parent.top : undefined
        style: Text.Outline
        styleColor: "#80000000"
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
