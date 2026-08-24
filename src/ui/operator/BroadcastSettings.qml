pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Editor do perfil de transmissão de uma saída. Só emite comandos; toda a
// validação e a persistência ficam no domínio.
ColumnLayout {
    id: root

    required property var controller
    //! Item da lista de telas, com `id` e o mapa `broadcast`.
    required property var screen
    property color textMain: "#eff6ff"
    property color textMuted: "#8da0bc"

    readonly property var profile: root.screen.broadcast || ({})
    readonly property bool transparent:
        (root.profile.backgroundMode || "chroma") === "transparent"

    spacing: 8

    function apply(changes) {
        root.controller.outputContext.setOutputBroadcastProfile(root.screen.id, changes)
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        Label { text: qsTr("Fundo"); color: root.textMuted; font.pixelSize: 12 }
        ComboBox {
            Layout.preferredWidth: 190
            model: [{"id": "chroma", "name": qsTr("Chroma key")},
                    {"id": "transparent", "name": qsTr("Transparente")}]
            textRole: "name"
            valueRole: "id"
            currentIndex: root.transparent ? 1 : 0
            onActivated: root.apply({"backgroundMode": currentValue})
        }
        Label { text: qsTr("Proporção"); color: root.textMuted; font.pixelSize: 12 }
        ComboBox {
            Layout.preferredWidth: 110
            model: ["16:9", "9:16"]
            currentIndex: (root.profile.aspectPreset || "16:9") === "9:16" ? 1 : 0
            onActivated: root.apply({"aspectPreset": currentValue})
        }
        Item { Layout.fillWidth: true }
    }

    RowLayout {
        Layout.fillWidth: true
        visible: !root.transparent
        spacing: 8
        Label { text: qsTr("Cor do chroma"); color: root.textMuted; font.pixelSize: 12 }
        Repeater {
            model: ["#00b140", "#0047bb", "#ff00ff", "#000000"]
            delegate: Rectangle {
                id: chromaSwatch
                required property string modelData
                width: 30
                height: 24
                radius: 5
                color: chromaSwatch.modelData
                border.width: (root.profile.chromaColor || "#00b140") === chromaSwatch.modelData
                              ? 2 : 1
                border.color: (root.profile.chromaColor || "#00b140") === chromaSwatch.modelData
                              ? "#ffffff" : "#38506f"
                TapHandler {
                    onTapped: root.apply({"chromaColor": chromaSwatch.modelData})
                }
            }
        }
        TextField {
            Layout.preferredWidth: 110
            text: root.profile.chromaColor || "#00b140"
            onEditingFinished: root.apply({"chromaColor": text})
        }
        Item { Layout.fillWidth: true }
    }

    Label {
        Layout.fillWidth: true
        visible: root.transparent && !root.controller.broadcastTransparencySupported
        text: root.controller.broadcastTransparencyWarning
        color: "#ffba70"
        font.pixelSize: 11
        wrapMode: Text.WordWrap
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 4
        columnSpacing: 8
        Repeater {
            model: [{"edge": "Left", "label": qsTr("Zona segura ←")},
                    {"edge": "Right", "label": qsTr("Zona segura →")},
                    {"edge": "Top", "label": qsTr("Zona segura ↑")},
                    {"edge": "Bottom", "label": qsTr("Zona segura ↓")}]
            delegate: RowLayout {
                id: safeAreaRow
                required property var modelData
                Layout.fillWidth: true
                Label {
                    text: safeAreaRow.modelData.label
                    color: root.textMuted
                    font.pixelSize: 11
                }
                SpinBox {
                    from: 0
                    to: 45
                    editable: true
                    value: root.profile["safeArea" + safeAreaRow.modelData.edge] === undefined
                           ? 5 : Math.round(root.profile["safeArea" + safeAreaRow.modelData.edge])
                    onValueModified: {
                        const changes = {}
                        changes["safeArea" + safeAreaRow.modelData.edge] = value
                        root.apply(changes)
                    }
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10
        Repeater {
            model: [{"key": "showClock", "label": qsTr("Relógio"), "fallback": false},
                    {"key": "showLowerThird", "label": qsTr("Lower third"), "fallback": true},
                    {"key": "showAlerts", "label": qsTr("Alertas"), "fallback": true},
                    {"key": "showAudienceMessage", "label": qsTr("Mensagens"), "fallback": true}]
            delegate: CheckBox {
                id: overlayToggle
                required property var modelData
                text: overlayToggle.modelData.label
                palette.windowText: root.textMain
                checked: root.profile[overlayToggle.modelData.key] === undefined
                         ? overlayToggle.modelData.fallback
                         : root.profile[overlayToggle.modelData.key]
                onClicked: {
                    const changes = {}
                    changes[overlayToggle.modelData.key] = checked
                    root.apply(changes)
                }
            }
        }
        Item { Layout.fillWidth: true }
    }

    BroadcastPreview {
        Layout.fillWidth: true
        Layout.preferredHeight: 150
        controller: root.controller
        profile: root.profile
    }
}
