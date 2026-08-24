pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window

// Host das saídas externas: posiciona a janela na tela correta e escolhe o
// renderizador do papel configurado. Nenhuma regra de apresentação vive aqui.
Window {
    id: root
    visible: false
    color: root.transparentBroadcast ? "transparent" : "black"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
           | Qt.WindowDoesNotAcceptFocus | Qt.Window
    title: qsTr("HolyScreen — %1").arg(root.outputDisplayName)

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
    //! Perfil de transmissão da saída, usado apenas no papel broadcast.
    property var broadcastProfile: ({})
    //! Verdadeiro quando a saída de transmissão pede fundo transparente.
    readonly property bool transparentBroadcast:
        root.outputRole === "broadcast"
        && (root.broadcastProfile.backgroundMode || "chroma") === "transparent"
    // O ApplicationController chega como propriedade de contexto criada pelo
    // main.cpp; o qmllint não conhece esse tipo, então a exceção fica restrita
    // a esta linha.
    // qmllint disable unqualified
    readonly property var controller: presentationController
    // qmllint enable unqualified
    //! Renderizador carregado para o papel atual. Exposto para testes.
    readonly property Item roleView: roleLoader.item as Item

    function placeOnTargetScreen() {
        if (root.targetScreenIndex < 0 || root.targetScreenIndex >= Application.screens.length)
            return

        // Native macOS fullscreen can create a Space on the primary display even
        // after assigning Window.screen. A borderless window using the monitor's
        // exact virtual-desktop geometry is deterministic on all three platforms.
        root.screen = Application.screens[root.targetScreenIndex]
        root.x = root.targetScreenX
        root.y = root.targetScreenY
        root.width = root.targetScreenWidth
        root.height = root.targetScreenHeight
        root.visible = true
        Qt.callLater(function() {
            console.info("output_placed", root.outputDisplayName,
                         "screenIndex=" + root.targetScreenIndex,
                         "geometry=" + root.x + "," + root.y + ","
                         + root.width + "x" + root.height)
        })
    }

    Component.onCompleted: Qt.callLater(root.placeOnTargetScreen)
    onTargetScreenIndexChanged: Qt.callLater(root.placeOnTargetScreen)
    onTargetScreenXChanged: Qt.callLater(root.placeOnTargetScreen)
    onTargetScreenYChanged: Qt.callLater(root.placeOnTargetScreen)
    onTargetScreenWidthChanged: Qt.callLater(root.placeOnTargetScreen)
    onTargetScreenHeightChanged: Qt.callLater(root.placeOnTargetScreen)

    Loader {
        id: roleLoader
        anchors.fill: parent
        sourceComponent: root.outputRole === "stage" ? stageComponent
                       : root.outputRole === "broadcast" ? broadcastComponent
                       : audienceComponent
    }

    Component {
        id: audienceComponent
        // Confidence e Custom ainda usam a composição de público até ganharem
        // renderizadores próprios.
        AudienceView {
            controller: root.controller
            bibleTranslationId: root.bibleTranslationId
            mediaEnabled: root.mediaEnabled
        }
    }

    Component {
        id: stageComponent
        StageOutputView {
            controller: root.controller
            bibleTranslationId: root.bibleTranslationId
            mediaEnabled: root.mediaEnabled
        }
    }

    Component {
        id: broadcastComponent
        BroadcastView {
            controller: root.controller
            bibleTranslationId: root.bibleTranslationId
            mediaEnabled: root.mediaEnabled
            profile: root.broadcastProfile
        }
    }

    Rectangle {
        anchors.fill: parent
        z: 1000
        visible: root.controller.outputContext.identifyVisible
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
