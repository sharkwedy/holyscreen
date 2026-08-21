pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: dashboard
    required property var controller
    signal openLibrary()
    signal openBible()
    signal importAudio()
    signal importVideo()
    signal importImage()

    readonly property color background: "#171b1f"
    readonly property color panel: "#20252a"
    readonly property color panelHigh: "#282e33"
    readonly property color line: "#41484e"
    readonly property color textMain: "#f2f4f5"
    readonly property color textMuted: "#aab2b8"
    readonly property color accent: "#b9c7ff"

    Rectangle { anchors.fill: parent; color: dashboard.background }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 14

        Rectangle {
            Layout.preferredWidth: Math.max(280, dashboard.width * 0.25)
            Layout.fillHeight: true
            color: dashboard.panel
            border.color: dashboard.line
            radius: 6

            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 92
                    color: "transparent"
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 12; spacing: 8
                        TextField {
                            id: mediaSearch
                            Layout.fillWidth: true
                            placeholderText: "Pesquisar mídia..."
                            leftPadding: 34
                            background: Rectangle { color: dashboard.panelHigh; border.color: dashboard.line; radius: 4 }
                            Label { anchors.left: parent.left; anchors.leftMargin: 10; anchors.verticalCenter: parent.verticalCenter; text: "⌕"; color: dashboard.textMuted }
                        }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 5
                            Repeater {
                                model: ["Letras", "Áudio", "Vídeo", "Imagem"]
                                Button {
                                    required property string modelData
                                    Layout.fillWidth: true
                                    text: modelData
                                    highlighted: modelData === "Áudio"
                                    onClicked: {
                                        if (modelData === "Áudio") dashboard.importAudio()
                                        else if (modelData === "Vídeo") dashboard.importVideo()
                                        else if (modelData === "Imagem") dashboard.importImage()
                                    }
                                }
                            }
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: dashboard.line }
                ListView {
                    id: libraryList
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true; model: controller.mediaPlaylist
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        width: ListView.view.width; height: 58
                        color: controller.currentMediaId === modelData.id ? "#343b43" : "transparent"
                        border.color: controller.currentMediaId === modelData.id ? dashboard.accent : "transparent"
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 10; spacing: 10
                            Label { text: modelData.type === "video" ? "▶" : "♫"; color: dashboard.accent; font.pixelSize: 18 }
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 1
                                Label { Layout.fillWidth: true; text: modelData.title; color: dashboard.textMain; elide: Text.ElideRight }
                                Label { text: dashboard.duration(modelData.durationMs) + "  •  " + modelData.typeLabel; color: dashboard.textMuted; font.pixelSize: 11 }
                            }
                        }
                        TapHandler { onDoubleTapped: controller.playMedia(modelData.id) }
                    }
                    Label { anchors.centerIn: parent; visible: libraryList.count === 0; text: "Importe ou arraste suas mídias"; color: dashboard.textMuted }
                }
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 42; color: dashboard.panelHigh
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 10
                        Label { text: libraryList.count + " itens"; color: dashboard.textMuted; font.pixelSize: 11 }
                        Item { Layout.fillWidth: true }
                        Button { text: "Sincronizar"; flat: true; onClicked: dashboard.openLibrary() }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 14
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: Math.max(350, dashboard.height * 0.53)
                color: dashboard.panel; border.color: dashboard.line; radius: 6
                ColumnLayout {
                    anchors.fill: parent; spacing: 0
                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        color: "#000000"; radius: 6
                        SimulatedOutput {
                            anchors.fill: parent
                            outputLabel: "PRÉVIA"
                            identifier: 1
                            wallpaper: controller.wallpaperColor
                            wallpaperSource: controller.wallpaperSource
                            wallpaperFit: controller.wallpaperFit
                            showClock: controller.clockVisible
                            clockText: controller.clockText
                            clockPosition: controller.clockPosition
                            clockFamily: controller.clockFontFamily
                            clockColor: controller.clockColor
                            isBlackout: controller.blackout
                            identifyVisible: controller.identifyVisible
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true; Layout.preferredHeight: 132
                        Layout.margins: 12; spacing: 6
                        Slider { Layout.fillWidth: true; from: 0; to: Math.max(1, controller.mediaDurationMs); value: controller.mediaPositionMs; onMoved: controller.seekMedia(value) }
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: dashboard.duration(controller.mediaPositionMs); color: dashboard.accent; font.pixelSize: 11 }
                            Item { Layout.fillWidth: true }
                            Label { text: dashboard.duration(controller.mediaDurationMs); color: dashboard.textMuted; font.pixelSize: 11 }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Button { text: "↝"; flat: true }
                            Button { text: "↻"; flat: true }
                            Item { Layout.fillWidth: true }
                            Button { text: "◀"; flat: true; onClicked: controller.previousMedia() }
                            Button { text: controller.mediaState === "playing" ? "Ⅱ" : "▶"; highlighted: true; onClicked: controller.toggleMediaPause() }
                            Button { text: "■"; flat: true; onClicked: controller.stopMedia() }
                            Button { text: "▶|"; flat: true; onClicked: controller.nextMedia() }
                            Item { Layout.fillWidth: true }
                            Label { text: "🔊"; color: dashboard.textMuted }
                            Slider { Layout.preferredWidth: 110; from: 0; to: 1; value: controller.mediaVolume; onMoved: controller.mediaVolume = value }
                        }
                    }
                }
            }
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                color: dashboard.panel; border.color: dashboard.line; radius: 6
                ColumnLayout {
                    anchors.fill: parent; spacing: 0
                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 42; color: dashboard.panelHigh
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 10
                            Label { text: "▾  Reprodução"; color: dashboard.textMain; font.bold: true; font.pixelSize: 12 }
                            Item { Layout.fillWidth: true }
                            Button { text: "Limpar"; flat: true; onClicked: controller.stopMedia() }
                        }
                    }
                    ListView {
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        model: controller.mediaPlaylist
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            width: ListView.view.width; height: 42
                            color: controller.currentMediaId === modelData.id ? "#263b55" : (index % 2 ? "#1d2226" : "transparent")
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
                                Label { text: index + 1; color: dashboard.textMuted; Layout.preferredWidth: 28 }
                                Label { Layout.fillWidth: true; text: modelData.title; color: dashboard.textMain; elide: Text.ElideRight }
                                Label { text: dashboard.duration(modelData.durationMs); color: dashboard.textMuted; font.pixelSize: 11 }
                                Button { text: "▶"; flat: true; onClicked: controller.playMedia(modelData.id) }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: Math.max(300, dashboard.width * 0.27)
            Layout.fillHeight: true
            color: dashboard.panel; border.color: dashboard.line; radius: 6
            ColumnLayout {
                anchors.fill: parent; spacing: 0
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 48; color: dashboard.panelHigh
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 10
                        Label { text: "▣  Bíblia Sagrada"; color: dashboard.textMain; font.bold: true }
                        Item { Layout.fillWidth: true }
                        Button { text: "☰"; flat: true; onClicked: dashboard.openBible() }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true; Layout.margins: 8
                    ComboBox { Layout.fillWidth: true; model: ["1 Pedro", "João", "Salmos"] }
                    SpinBox { from: 1; to: 150; value: 4 }
                    ComboBox { Layout.preferredWidth: 82; model: ["ARC", "NVI"] }
                }
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: dashboard.line }
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 0
                    model: [
                        "Aos presbíteros que estão entre vós, admoesto eu, que sou também presbítero com eles, e testemunha das aflições de Cristo, e participante da glória que se há de revelar:",
                        "apascentai o rebanho de Deus que está entre vós, tendo cuidado dele, não por força, mas voluntariamente; nem por torpe ganância, mas de ânimo pronto;",
                        "nem como tendo domínio sobre a herança de Deus, mas servindo de exemplo ao rebanho.",
                        "E, quando aparecer o Sumo Pastor, alcançareis a incorruptível coroa de glória."
                    ]
                    delegate: Rectangle {
                        required property string modelData
                        required property int index
                        width: ListView.view.width
                        height: verseText.implicitHeight + 30
                        color: index % 2 ? dashboard.panelHigh : "transparent"
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 10; spacing: 10
                            Label { text: index + 1; color: dashboard.accent; font.bold: true; Layout.alignment: Qt.AlignTop }
                            Label { id: verseText; Layout.fillWidth: true; text: modelData; color: dashboard.textMain; wrapMode: Text.WordWrap; lineHeight: 1.25 }
                        }
                    }
                }
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 48; color: dashboard.panelHigh
                    ColumnLayout {
                        anchors.centerIn: parent; spacing: 1
                        Label { text: "Almeida Revista e Corrigida (ARC)"; color: dashboard.textMain; font.bold: true; font.pixelSize: 10 }
                        Label { text: "© 2009 Sociedade Bíblica do Brasil"; color: dashboard.textMuted; font.pixelSize: 9 }
                    }
                }
            }
        }
    }

    function duration(milliseconds) {
        const totalSeconds = Math.max(0, Math.floor((milliseconds || 0) / 1000))
        const minutes = Math.floor(totalSeconds / 60)
        const seconds = totalSeconds % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }
}
