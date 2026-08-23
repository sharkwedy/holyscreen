import QtQuick
import QtMultimedia

// Composição da saída de transmissão. Nesta etapa ela apenas roteia o mesmo
// conteúdo sem wallpaper e sobre um fundo sólido; fundo transparente, chroma,
// zonas seguras e seleção de overlays chegam com o perfil de Broadcast.
Item {
    id: root

    property var controller
    property string bibleTranslationId: ""
    property bool mediaEnabled: true
    property color backgroundColor: "#000000"

    Rectangle {
        anchors.fill: parent
        color: root.backgroundColor
    }

    PresentationImageLayer {
        anchors.fill: parent
        controller: root.controller
        isBlackout: root.controller.blackout
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        z: 60
        visible: root.mediaEnabled && root.controller.videoVisible
                 && !root.controller.blackout
        fillMode: VideoOutput.PreserveAspectFit
        Component.onCompleted: root.controller.registerVideoSink(videoOutput.videoSink)
        Component.onDestruction: root.controller.unregisterVideoSink(videoOutput.videoSink)
    }

    PresentationTextLayer {
        anchors.fill: parent
        controller: root.controller
        isBlackout: root.controller.blackout
        textOverride: root.controller.currentPresentationType === "bible"
                      ? root.controller.bibleTextForSlide(
                            root.controller.currentSlideIndex, root.bibleTranslationId)
                      : ""
    }

    OutputClock {
        controller: root.controller
        isBlackout: root.controller.blackout
    }

    LiveOverlays {
        anchors.fill: parent
        z: 80
        visible: !root.controller.blackout
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
