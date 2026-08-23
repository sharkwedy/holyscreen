import QtQuick
import QtMultimedia

// Composição da saída de palco: texto atual e próximo, relógio, mensagens e
// cronômetros para a equipe, com o vídeo em primeiro plano quando ativo.
Item {
    id: root

    property var controller
    property string bibleTranslationId: ""
    property bool mediaEnabled: true

    Rectangle {
        anchors.fill: parent
        color: root.controller.blackout ? "#000000" : root.controller.wallpaperColor
    }

    StageView {
        anchors.fill: parent
        z: 100
        visible: !root.controller.identifyVisible
                 && !(root.mediaEnabled && root.controller.videoVisible)
        currentText: root.controller.currentPresentationType === "bible"
                     ? root.controller.bibleTextForSlide(
                           root.controller.currentSlideIndex, root.bibleTranslationId)
                     : root.controller.currentSlideText
        nextText: root.controller.currentPresentationType === "bible"
                  ? root.controller.bibleTextForSlide(
                        root.controller.currentSlideIndex + 1, root.bibleTranslationId)
                  : root.controller.nextSlideText
        clockText: root.controller.clockText
        message: root.controller.stageMessage
        countdownText: root.controller.countdownText
        countdownRunning: root.controller.countdownRunning
        stopwatchText: root.controller.stopwatchText
        stopwatchRunning: root.controller.stopwatchRunning
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        z: 110
        visible: root.mediaEnabled && root.controller.videoVisible
                 && !root.controller.blackout
        fillMode: VideoOutput.PreserveAspectFit
        Component.onCompleted: root.controller.registerVideoSink(videoOutput.videoSink)
        Component.onDestruction: root.controller.unregisterVideoSink(videoOutput.videoSink)
    }
}
