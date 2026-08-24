import QtQuick
import QtMultimedia

// Composição da saída de público: wallpaper, imagem, vídeo, texto, relógio e
// overlays ao vivo. É carregada pela OutputWindow conforme o papel da saída.
Item {
    id: root

    property var controller
    property string bibleTranslationId: ""
    property bool mediaEnabled: true

    Rectangle {
        anchors.fill: parent
        color: root.controller.outputContext.blackout ? "#000000" : root.controller.wallpaperColor
    }

    Image {
        anchors.fill: parent
        visible: !root.controller.outputContext.blackout && source.toString().length > 0
        source: root.controller.wallpaperSource
        asynchronous: true
        cache: true
        fillMode: root.controller.wallpaperFit === "contain" ? Image.PreserveAspectFit
                  : root.controller.wallpaperFit === "stretch" ? Image.Stretch
                  : root.controller.wallpaperFit === "center" ? Image.Pad
                  : Image.PreserveAspectCrop
    }

    PresentationImageLayer {
        anchors.fill: parent
        controller: root.controller
        isBlackout: root.controller.outputContext.blackout
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        z: 60
        visible: root.mediaEnabled && root.controller.videoVisible
                 && !root.controller.outputContext.blackout
        fillMode: VideoOutput.PreserveAspectFit
        Component.onCompleted: root.controller.registerVideoSink(videoOutput.videoSink)
        Component.onDestruction: root.controller.unregisterVideoSink(videoOutput.videoSink)
    }

    PresentationTextLayer {
        anchors.fill: parent
        controller: root.controller
        isBlackout: root.controller.outputContext.blackout
        textOverride: root.controller.currentPresentationType === "bible"
                      ? root.controller.bibleTextForSlide(
                            root.controller.currentSlideIndex, root.bibleTranslationId)
                      : ""
    }

    OutputClock {
        controller: root.controller
        isBlackout: root.controller.outputContext.blackout
    }

    LiveOverlays {
        anchors.fill: parent
        z: 80
        visible: !root.controller.outputContext.blackout
        message: root.controller.audienceMessage
        alertMessage: root.controller.alertMessage
        lowerThirdTitle: root.controller.lowerThirdTitle
        lowerThirdSubtitle: root.controller.lowerThirdSubtitle
        countdownText: root.controller.countdownText
        countdownRunning: root.controller.countdownRunning
        stopwatchText: root.controller.stopwatchText
        stopwatchRunning: root.controller.stopwatchRunning
    }
}
