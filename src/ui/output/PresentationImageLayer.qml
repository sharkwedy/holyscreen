// qmllint disable unqualified
import QtQuick

Item {
    id: root
    property int activeLayer: 0
    property url requestedSource: presentationController.presentationImageSource
    property bool isBlackout: false

    visible: true
    opacity: presentationController.imageVisible && !isBlackout ? 1 : 0

    function fitMode() {
        return presentationController.imageFit === "cover" ? Image.PreserveAspectCrop
             : presentationController.imageFit === "stretch" ? Image.Stretch
             : presentationController.imageFit === "center" ? Image.Pad
             : Image.PreserveAspectFit
    }

    function transitionTo(nextSource) {
        const active = activeLayer === 0 ? layerA : layerB
        if (active.source.toString() === nextSource.toString())
            return
        if (presentationController.imageTransition !== "fade" || !presentationController.imageVisible) {
            active.source = nextSource
            layerA.opacity = activeLayer === 0 ? 1 : 0
            layerB.opacity = activeLayer === 1 ? 1 : 0
            return
        }
        if (activeLayer === 0) {
            layerB.source = nextSource
            fadeToB.restart()
        } else {
            layerA.source = nextSource
            fadeToA.restart()
        }
    }

    onRequestedSourceChanged: transitionTo(requestedSource)
    Component.onCompleted: {
        layerA.source = requestedSource
        layerA.opacity = 1
    }

    Behavior on opacity {
        NumberAnimation {
            duration: presentationController.imageTransition === "fade" ? 350 : 0
            easing.type: Easing.InOutQuad
        }
    }

    Image {
        id: layerA
        anchors.fill: parent
        asynchronous: true
        cache: true
        fillMode: root.fitMode()
        opacity: 1
    }

    Image {
        id: layerB
        anchors.fill: parent
        asynchronous: true
        cache: true
        fillMode: root.fitMode()
        opacity: 0
    }

    ParallelAnimation {
        id: fadeToB
        NumberAnimation { target: layerA; property: "opacity"; to: 0; duration: 350 }
        NumberAnimation { target: layerB; property: "opacity"; to: 1; duration: 350 }
        onFinished: root.activeLayer = 1
    }

    ParallelAnimation {
        id: fadeToA
        NumberAnimation { target: layerB; property: "opacity"; to: 0; duration: 350 }
        NumberAnimation { target: layerA; property: "opacity"; to: 1; duration: 350 }
        onFinished: root.activeLayer = 0
    }
}
