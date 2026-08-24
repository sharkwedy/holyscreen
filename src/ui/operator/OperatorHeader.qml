pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ToolBar {
    id: header

    required property var controller
    signal openLive()
    signal openEvents()
    signal openLibrary()
    signal openIntegrations()
    signal openAutomations()
    signal openSettings()
    signal toggleFullScreen()

    function playFavorite(path) {
        const mediaId = header.controller.addCatalogFileToPlaylist(path)
        if (mediaId.length > 0)
            header.controller.playMedia(mediaId)
    }

    height: 92
    background: Rectangle { color: "#15191d"; border.color: "#353b40" }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            Layout.leftMargin: 18
            Layout.rightMargin: 14
            spacing: 12

            Label {
                text: qsTr("HolyScreen")
                color: "#f2f4f5"
                font.bold: true
                font.pixelSize: 15
            }
            ToolButton {
                text: qsTr("Live")
                Accessible.name: qsTr("Abrir comunicação ao vivo")
                onClicked: header.openLive()
            }
            ToolButton {
                text: qsTr("Operação")
                enabled: false
                Accessible.name: qsTr("Área de operação atual")
            }
            ToolButton {
                text: qsTr("Agenda")
                Accessible.name: qsTr("Abrir agenda do culto")
                onClicked: header.openEvents()
            }
            ToolButton {
                text: qsTr("Biblioteca")
                Accessible.name: qsTr("Abrir biblioteca de mídia")
                onClicked: header.openLibrary()
            }
            ToolButton {
                text: qsTr("Integrações")
                Accessible.name: qsTr("Abrir integrações")
                onClicked: header.openIntegrations()
            }
            ToolButton {
                text: qsTr("Automações")
                Accessible.name: qsTr("Abrir automações")
                onClicked: header.openAutomations()
                ToolTip.visible: hovered && !header.controller.automationsEnabled
                ToolTip.text: qsTr("Automações pausadas")
            }
            Item { Layout.fillWidth: true }
            ToolButton {
                text: "⚙"
                Accessible.name: qsTr("Configurações")
                onClicked: header.openSettings()
            }
            ToolButton {
                text: "⛶"
                Accessible.name: qsTr("Alternar tela cheia")
                onClicked: header.toggleFullScreen()
            }
            Button {
                text: header.controller.outputContext.blackout ? qsTr("Restaurar") : qsTr("Ao vivo")
                Accessible.name: header.controller.outputContext.blackout
                                 ? qsTr("Restaurar apresentação")
                                 : qsTr("Manter apresentação ao vivo")
                highlighted: true
                onClicked: header.controller.outputContext.blackout = false
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "#1d2227"
            border.color: "#353b40"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 14
                spacing: 10

                Label {
                    text: qsTr("★  FAVORITOS")
                    color: "#c7d2fe"
                    font.bold: true
                    font.pixelSize: 11
                }
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    Layout.topMargin: 8
                    Layout.bottomMargin: 8
                    color: "#41484e"
                }
                ListView {
                    id: favoriteMediaList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: ListView.Horizontal
                    spacing: 6
                    clip: true
                    model: header.controller.favoriteMedia

                    delegate: Button {
                        required property var modelData
                        height: 32
                        width: Math.min(220, Math.max(110, implicitWidth))
                        y: (favoriteMediaList.height - height) / 2
                        text: (modelData.type === "video" ? "▶  "
                              : modelData.type === "image" ? "▧  " : "♫  ")
                              + (modelData.fileName || modelData.title || qsTr("Sem título"))
                        font.pixelSize: 11
                        Accessible.name: qsTr("Reproduzir favorito %1")
                                         .arg(modelData.fileName || modelData.title || qsTr("Sem título"))
                        onClicked: header.playFavorite(modelData.path)
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Reproduzir %1").arg(modelData.fileName || modelData.title)
                    }
                    Label {
                        anchors.centerIn: parent
                        visible: favoriteMediaList.count === 0
                        text: qsTr("Clique com o botão direito em uma mídia para adicioná-la")
                        color: "#8d979f"
                        font.pixelSize: 11
                    }
                }
            }
        }
    }
}
