pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: panel

    required property var applicationController
    required property var mediaController
    required property var outputController
    required property color panelColor
    required property color lineColor
    required property color textMainColor
    required property color textMutedColor
    required property color accentColor

    property bool screenControlsExpanded: true
    property real mediaVolumeBeforeMute: 0.8
    property var screenBeingRenamed: null

    color: panel.panelColor
    border.color: panel.lineColor
    radius: 6

    function duration(milliseconds) {
        const totalSeconds = Math.max(0, Math.floor((milliseconds || 0) / 1000))
        const minutes = Math.floor(totalSeconds / 60)
        const seconds = totalSeconds % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    function externalScreenCount() {
        let count = 0
        for (let index = 0; index < panel.outputController.screens.length; ++index) {
            if (!panel.outputController.screens[index].primary)
                ++count
        }
        return count
    }

    function ensureExternalOutputs() {
        if (panel.outputController.outputWindows.length === 0 && externalScreenCount() > 0)
            panel.outputController.enableAllScreens()
    }

    function setScreenMediaEnabled(screen, enabled) {
        if (enabled && !screen.selected) {
            const screenId = screen.id
            if (panel.outputController.toggleScreen(screenId, true)) {
                Qt.callLater(function() {
                    panel.outputController.setOutputMediaEnabled(screenId, true)
                })
            }
            return
        }
        if (screen.selected)
            panel.outputController.setOutputMediaEnabled(screen.id, enabled)
    }

    function openScreenRename(screen) {
        if (!screen.selected && !panel.outputController.toggleScreen(screen.id, true))
            return
        panel.screenBeingRenamed = screen
        screenNameField.text = screen.name
        screenRenameDialog.open()
        screenNameField.forceActiveFocus()
        screenNameField.selectAll()
    }

    Component.onCompleted: Qt.callLater(ensureExternalOutputs)

    Connections {
        target: panel.outputController
        function onScreensChanged() { Qt.callLater(panel.ensureExternalOutputs) }
    }

    Dialog {
        id: screenRenameDialog
        title: qsTr("Renomear monitor")
        modal: true
        anchors.centerIn: parent
        width: 430
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            if (panel.screenBeingRenamed && screenNameField.text.trim().length > 0) {
                panel.outputController.setOutputDisplayName(
                            panel.screenBeingRenamed.id, screenNameField.text)
            }
        }
        contentItem: ColumnLayout {
            spacing: 10
            Label {
                text: qsTr("Nome que será exibido no HolyScreen:")
                color: panel.textMainColor
            }
            TextField {
                id: screenNameField
                Layout.fillWidth: true
                placeholderText: qsTr("Ex.: Projetor principal")
                onAccepted: screenRenameDialog.accept()
            }
            Label {
                text: qsTr("O nome técnico do monitor no Windows não será alterado.")
                color: panel.textMutedColor
                font.pixelSize: UiScale.px(11)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#000000"
            radius: 6
            SimulatedOutput {
                anchors.fill: parent
                controller: panel.applicationController
                outputLabel: qsTr("PRÉVIA")
                identifier: 1
                wallpaper: panel.applicationController.wallpaperColor
                wallpaperSource: panel.applicationController.wallpaperSource
                wallpaperFit: panel.applicationController.wallpaperFit
                showClock: panel.applicationController.clockVisible
                clockText: panel.applicationController.clockText
                clockPosition: panel.applicationController.clockPosition
                clockFamily: panel.applicationController.clockFontFamily
                clockFontSize: panel.applicationController.clockFontSize
                clockColor: panel.applicationController.clockColor
                clockFontBold: panel.applicationController.clockFontBold
                clockFontItalic: panel.applicationController.clockFontItalic
                clockBackgroundColor: panel.applicationController.clockBackgroundColor
                clockLineHeight: panel.applicationController.clockLineHeight
                clockCornerRadius: panel.applicationController.clockCornerRadius
                clockTextOpacity: panel.applicationController.clockTextOpacity
                clockBackgroundOpacity: panel.applicationController.clockBackgroundOpacity
                clockMarginHorizontal: panel.applicationController.clockMarginHorizontal
                clockMarginVertical: panel.applicationController.clockMarginVertical
                clockEffect: panel.applicationController.clockEffect
                isBlackout: panel.outputController.blackout
                identifyVisible: panel.outputController.identifyVisible
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: panel.screenControlsExpanded ? 202 : 148
            Layout.margins: 12
            spacing: 6

            Slider {
                Layout.fillWidth: true
                from: 0
                to: Math.max(1, panel.mediaController.mediaDurationMs)
                value: panel.mediaController.mediaPositionMs
                onMoved: panel.mediaController.seekMedia(value)
            }
            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: panel.duration(panel.mediaController.mediaPositionMs)
                    color: panel.accentColor
                    font.pixelSize: UiScale.px(11)
                }
                Item { Layout.fillWidth: true }
                Label {
                    text: panel.duration(panel.mediaController.mediaDurationMs)
                    color: panel.textMutedColor
                    font.pixelSize: UiScale.px(11)
                }
            }
            RowLayout {
                Layout.fillWidth: true
                PlayerButton {
                    text: "⇄"
                    Accessible.name: qsTr("Embaralhar playlist")
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                    onClicked: panel.mediaController.shuffleMediaPlaylist()
                }
                PlayerButton {
                    text: panel.mediaController.mediaRepeatMode === "one" ? "↻¹" : "↻"
                    highlighted: panel.mediaController.mediaRepeatMode !== "off"
                    Accessible.name: panel.mediaController.mediaRepeatMode === "off"
                                     ? qsTr("Ativar repetição de toda a playlist")
                                     : panel.mediaController.mediaRepeatMode === "all"
                                       ? qsTr("Repetir somente o item atual")
                                       : qsTr("Desativar repetição")
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                    onClicked: panel.mediaController.mediaRepeatMode =
                                   panel.mediaController.mediaRepeatMode === "off" ? "all"
                                 : panel.mediaController.mediaRepeatMode === "all" ? "one" : "off"
                }
                PlayerButton {
                    text: "⇢"
                    highlighted: panel.mediaController.mediaSmoothTransition
                    Accessible.name: panel.mediaController.mediaSmoothTransition
                                     ? qsTr("Desativar transição suave entre faixas")
                                     : qsTr("Ativar transição suave entre faixas")
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                    onClicked: panel.mediaController.mediaSmoothTransition =
                               !panel.mediaController.mediaSmoothTransition
                }
                Item { Layout.fillWidth: true }
                PlayerButton {
                    text: "⏮"
                    Accessible.name: qsTr("Mídia anterior")
                    onClicked: panel.mediaController.previousMedia()
                }
                PlayerButton {
                    text: panel.mediaController.mediaState === "playing"
                          || panel.mediaController.mediaState === "buffering" ? "⏸" : "▶"
                    highlighted: true
                    Accessible.name: panel.mediaController.mediaState === "playing"
                                     || panel.mediaController.mediaState === "buffering"
                                     ? qsTr("Pausar") : qsTr("Reproduzir")
                    onClicked: panel.mediaController.toggleMediaPause()
                }
                PlayerButton {
                    text: "■"
                    Accessible.name: qsTr("Parar")
                    onClicked: panel.mediaController.stopMedia()
                }
                PlayerButton {
                    text: "⏭"
                    Accessible.name: qsTr("Próxima mídia")
                    onClicked: panel.mediaController.nextMedia()
                }
                Item { Layout.fillWidth: true }
                PlayerButton {
                    implicitWidth: 42
                    text: panel.mediaController.mediaVolume > 0.001 ? "🔊" : "🔇"
                    Accessible.name: panel.mediaController.mediaVolume > 0.001
                                     ? qsTr("Mutar") : qsTr("Desmutar")
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                    onClicked: {
                        if (panel.mediaController.mediaVolume > 0.001) {
                            panel.mediaVolumeBeforeMute = panel.mediaController.mediaVolume
                            panel.mediaController.mediaVolume = 0
                        } else {
                            panel.mediaController.mediaVolume = Math.max(
                                        0.05, panel.mediaVolumeBeforeMute)
                        }
                    }
                }
                Slider {
                    Layout.preferredWidth: 110
                    from: 0
                    to: 1
                    value: panel.mediaController.mediaVolume
                    onMoved: panel.mediaController.mediaVolume = value
                }
                PlayerButton {
                    implicitWidth: 42
                    text: panel.screenControlsExpanded ? "▾" : "▸"
                    Accessible.name: panel.screenControlsExpanded
                                     ? qsTr("Ocultar seleção de telas")
                                     : qsTr("Mostrar seleção de telas")
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                    onClicked: panel.screenControlsExpanded = !panel.screenControlsExpanded
                }
            }

            Rectangle {
                visible: panel.screenControlsExpanded
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                radius: 6
                color: "#2b3137"
                border.color: panel.lineColor
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 8
                    spacing: 12
                    Label {
                        text: qsTr("Exibir vídeo em:")
                        color: panel.textMainColor
                        font.bold: true
                        font.pixelSize: UiScale.px(12)
                    }
                    Repeater {
                        model: panel.outputController.screens
                        delegate: CheckBox {
                            id: screenCheckBox
                            required property var modelData
                            visible: !screenCheckBox.modelData.primary
                            text: screenCheckBox.modelData.name
                            checked: screenCheckBox.modelData.selected
                                     && screenCheckBox.modelData.mediaEnabled
                            spacing: 7
                            indicator: Rectangle {
                                implicitWidth: 20
                                implicitHeight: 20
                                x: screenCheckBox.leftPadding
                                y: (screenCheckBox.height - height) / 2
                                radius: 4
                                color: screenCheckBox.checked ? "#4f7cff" : "#171b1f"
                                border.color: screenCheckBox.checked ? "#8eabff" : "#77818a"
                                Label {
                                    anchors.centerIn: parent
                                    text: "✓"
                                    visible: screenCheckBox.checked
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: UiScale.px(14)
                                }
                            }
                            contentItem: Label {
                                leftPadding: screenCheckBox.indicator.width + screenCheckBox.spacing
                                text: screenCheckBox.text
                                color: panel.textMainColor
                                font.pixelSize: UiScale.px(12)
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Clique com o botão direito para renomear")
                            onClicked: panel.setScreenMediaEnabled(
                                           screenCheckBox.modelData, checked)
                            TapHandler {
                                acceptedButtons: Qt.RightButton
                                onTapped: panel.openScreenRename(screenCheckBox.modelData)
                            }
                        }
                    }
                    Label {
                        visible: panel.externalScreenCount() === 0
                        text: qsTr("nenhuma tela externa detectada")
                        color: panel.textMutedColor
                        font.pixelSize: UiScale.px(12)
                    }
                    Button {
                        visible: panel.externalScreenCount()
                                 > panel.outputController.outputWindows.length
                        text: qsTr("Ativar todas")
                        onClicked: panel.outputController.enableAllScreens()
                    }
                    Item { Layout.fillWidth: true }
                }
            }
        }
    }
}
