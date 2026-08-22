// qmllint disable unqualified
import QtQuick
import QtQuick.Window
import QtMultimedia

Window {
    id: root
    visible: false
    color: "black"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
           | Qt.WindowDoesNotAcceptFocus | Qt.Window
    title: "HolyScreen — " + outputDisplayName

    property int targetScreenIndex: -1
    property int targetScreenX: 0
    property int targetScreenY: 0
    property int targetScreenWidth: 1920
    property int targetScreenHeight: 1080
    property string outputDisplayName: ""
    property int identifier: 1
    property string bibleTranslationId: ""
    property string outputRole: "audience"
    property bool mediaEnabled: true

    function placeOnTargetScreen() {
        if (targetScreenIndex < 0 || targetScreenIndex >= Application.screens.length)
            return

        // Native macOS fullscreen can create a Space on the primary display even
        // after assigning Window.screen. A borderless window using the monitor's
        // exact virtual-desktop geometry is deterministic on all three platforms.
        screen = Application.screens[targetScreenIndex]
        x = targetScreenX
        y = targetScreenY
        width = targetScreenWidth
        height = targetScreenHeight
        visible = true
        Qt.callLater(function() {
            console.info("output_placed", outputDisplayName,
                         "screen=" + screen.name,
                         "geometry=" + x + "," + y + "," + width + "x" + height)
        })
    }

    Component.onCompleted: Qt.callLater(placeOnTargetScreen)
    onTargetScreenIndexChanged: Qt.callLater(placeOnTargetScreen)
    onTargetScreenXChanged: Qt.callLater(placeOnTargetScreen)
    onTargetScreenYChanged: Qt.callLater(placeOnTargetScreen)
    onTargetScreenWidthChanged: Qt.callLater(placeOnTargetScreen)
    onTargetScreenHeightChanged: Qt.callLater(placeOnTargetScreen)

    Rectangle {
        anchors.fill: parent
        color: presentationController.blackout ? "#000000" : presentationController.wallpaperColor
    }

    StageView {
        anchors.fill: parent
        z: 100
        visible: root.outputRole === "stage" && !presentationController.identifyVisible
                 && !(root.mediaEnabled && presentationController.videoVisible)
        currentText: presentationController.currentPresentationType === "bible"
                     ? presentationController.bibleTextForSlide(
                           presentationController.currentSlideIndex, root.bibleTranslationId)
                     : presentationController.currentSlideText
        nextText: presentationController.currentPresentationType === "bible"
                  ? presentationController.bibleTextForSlide(
                        presentationController.currentSlideIndex + 1, root.bibleTranslationId)
                  : presentationController.nextSlideText
        clockText: presentationController.clockText
        message: presentationController.stageMessage
        countdownText: presentationController.countdownText
        countdownRunning: presentationController.countdownRunning
        stopwatchText: presentationController.stopwatchText
        stopwatchRunning: presentationController.stopwatchRunning
    }

    Image {
        anchors.fill: parent
        visible: !presentationController.blackout && source.toString().length > 0
        source: presentationController.wallpaperSource
        asynchronous: true
        cache: true
        fillMode: presentationController.wallpaperFit === "contain" ? Image.PreserveAspectFit
                  : presentationController.wallpaperFit === "stretch" ? Image.Stretch
                  : presentationController.wallpaperFit === "center" ? Image.Pad
                  : Image.PreserveAspectCrop
    }

    PresentationImageLayer {
        anchors.fill: parent
        isBlackout: presentationController.blackout
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        z: root.outputRole === "stage" ? 110 : 60
        visible: root.mediaEnabled && presentationController.videoVisible
                 && !presentationController.blackout
        fillMode: VideoOutput.PreserveAspectFit
        Component.onCompleted: presentationController.registerVideoSink(videoSink)
        Component.onDestruction: presentationController.unregisterVideoSink(videoSink)
    }

    PresentationTextLayer {
        anchors.fill: parent
        isBlackout: presentationController.blackout
        textOverride: presentationController.currentPresentationType === "bible"
                      ? presentationController.bibleTextForSlide(
                            presentationController.currentSlideIndex, root.bibleTranslationId)
                      : ""
    }

    Item {
        id: clockPanel
        visible: presentationController.clockVisible && !presentationController.blackout
        width: clockText.implicitWidth + 28
        height: clockText.implicitHeight + 18
        x: presentationController.clockPosition.endsWith("Left")
           ? 40 + parent.width * presentationController.clockMarginHorizontal / 100
           : presentationController.clockPosition.endsWith("Right")
             ? parent.width - width - 40
               + parent.width * presentationController.clockMarginHorizontal / 100
             : (parent.width - width) / 2
               + parent.width * presentationController.clockMarginHorizontal / 100
        y: presentationController.clockPosition.startsWith("top")
           ? 40 + parent.height * presentationController.clockMarginVertical / 100
           : presentationController.clockPosition.startsWith("bottom")
             ? parent.height - height - 40
               + parent.height * presentationController.clockMarginVertical / 100
             : (parent.height - height) / 2
               + parent.height * presentationController.clockMarginVertical / 100
        Rectangle {
            anchors.fill: parent
            color: presentationController.clockBackgroundColor
            opacity: presentationController.clockBackgroundOpacity
            radius: presentationController.clockCornerRadius
        }
        Text {
            id: clockText
            anchors.centerIn: parent
            text: presentationController.clockText
            color: presentationController.clockColor
            opacity: presentationController.clockTextOpacity
            font.bold: presentationController.clockFontBold
            font.italic: presentationController.clockFontItalic
            font.family: presentationController.clockFontFamily
            font.pixelSize: presentationController.clockFontSize
            lineHeight: presentationController.clockLineHeight
            lineHeightMode: Text.ProportionalHeight
            style: presentationController.clockEffect === "outline" ? Text.Outline
                 : presentationController.clockEffect === "raised" ? Text.Raised
                 : presentationController.clockEffect === "sunken" ? Text.Sunken
                 : Text.Normal
            styleColor: "#b0000000"
        }
    }

    LiveOverlays {
        anchors.fill: parent
        z: 80
        visible: root.outputRole === "audience" && !presentationController.blackout
        message: presentationController.audienceMessage
        alertMessage: presentationController.alertMessage
        lowerThirdTitle: presentationController.lowerThirdTitle
        lowerThirdSubtitle: presentationController.lowerThirdSubtitle
        countdownText: presentationController.countdownText
        countdownRunning: presentationController.countdownRunning
        stopwatchText: presentationController.stopwatchText
        stopwatchRunning: presentationController.stopwatchRunning
    }

    Rectangle {
        anchors.fill: parent
        visible: presentationController.identifyVisible
        color: "#0f172a"
        opacity: 0.92
        Text {
            anchors.centerIn: parent
            text: root.identifier + "\n" + root.outputDisplayName
            horizontalAlignment: Text.AlignHCenter
            color: "white"
            font.bold: true
            font.pixelSize: Math.min(parent.width, parent.height) * 0.22
        }
    }

    HoverHandler { cursorShape: Qt.BlankCursor }
}
