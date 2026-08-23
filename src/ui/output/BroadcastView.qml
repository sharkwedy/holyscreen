import QtQuick
import QtMultimedia

// Composição da saída de transmissão. O fundo é transparente ou chroma, a
// mídia ocupa a caixa de composição do preset e texto e overlays respeitam a
// zona segura configurada.
Item {
    id: root

    property var controller
    property string bibleTranslationId: ""
    property bool mediaEnabled: true
    //! Perfil serializado por BroadcastProfile (backgroundMode, chromaColor,
    //! safeArea*, aspectRatio e as chaves show*).
    property var profile: ({})

    readonly property bool transparentBackground:
        (root.profile.backgroundMode || "chroma") === "transparent"
    readonly property color chromaColor: root.profile.chromaColor || "#00b140"
    readonly property real aspectRatio: root.profile.aspectRatio || (16 / 9)
    readonly property bool contentVisible: !root.controller.blackout

    function safeMargin(edge, extent) {
        const percent = root.profile["safeArea" + edge]
        return extent * (percent === undefined ? 5 : percent) / 100
    }

    Rectangle {
        objectName: "broadcastBackground"
        anchors.fill: parent
        visible: !root.transparentBackground
        color: root.chromaColor
    }

    // Caixa de composição na proporção do preset, centralizada na tela real.
    Item {
        id: contentBox
        anchors.centerIn: parent
        width: parent.width / parent.height > root.aspectRatio
               ? parent.height * root.aspectRatio : parent.width
        height: parent.width / parent.height > root.aspectRatio
                ? parent.height : parent.width / root.aspectRatio

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

        // Zona segura de texto e overlays.
        Item {
            id: safeArea
            objectName: "broadcastSafeArea"
            anchors.fill: parent
            anchors.leftMargin: root.safeMargin("Left", contentBox.width)
            anchors.rightMargin: root.safeMargin("Right", contentBox.width)
            anchors.topMargin: root.safeMargin("Top", contentBox.height)
            anchors.bottomMargin: root.safeMargin("Bottom", contentBox.height)

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
                isBlackout: root.controller.blackout || !(root.profile.showClock === true)
            }

            LiveOverlays {
                objectName: "broadcastOverlays"
                anchors.fill: parent
                z: 80
                visible: root.contentVisible
                message: root.profile.showAudienceMessage === false
                         ? "" : root.controller.audienceMessage
                alertMessage: root.profile.showAlerts === false
                              ? "" : root.controller.alertMessage
                lowerThirdTitle: root.profile.showLowerThird === false
                                 ? "" : root.controller.lowerThirdTitle
                lowerThirdSubtitle: root.profile.showLowerThird === false
                                    ? "" : root.controller.lowerThirdSubtitle
                countdownText: root.controller.countdownText
                countdownRunning: root.controller.countdownRunning
                stopwatchText: root.controller.stopwatchText
                stopwatchRunning: root.controller.stopwatchRunning
            }
        }
    }
}
