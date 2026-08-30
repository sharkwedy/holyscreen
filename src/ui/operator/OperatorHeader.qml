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

    readonly property var favoriteItems: {
        const result = []
        const media = header.controller.mediaContext.favoriteMedia
        for (let index = 0; index < media.length; ++index) {
            const item = media[index]
            result.push({"kind": "media", "path": item.path,
                         "type": item.type, "title": item.fileName || item.title,
                         "thumbnailSource": item.thumbnailSource || ""})
        }
        const verses = header.controller.bibleContext.favoriteBibleVerses
        for (let verseIndex = 0; verseIndex < verses.length; ++verseIndex) {
            const verse = verses[verseIndex]
            result.push({"kind": "bible", "title": verse.label,
                         "text": verse.text, "bookId": verse.bookId,
                         "chapter": verse.chapter, "verse": verse.verse})
        }
        return result
    }

    function activateFavorite(item) {
        if (item.kind === "bible") {
            header.controller.bibleContext.presentBibleReference(
                        item.bookId, item.chapter, item.verse)
            return
        }
        const mediaId = header.controller.mediaContext.addCatalogFileToPlaylist(item.path)
        if (mediaId.length > 0)
            header.controller.mediaContext.playMedia(mediaId)
    }

    height: 108
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
                font.pixelSize: UiScale.px(15)
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
            Layout.preferredHeight: 60
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
                    font.pixelSize: UiScale.px(11)
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
                    model: header.favoriteItems

                    delegate: Button {
                        id: favoriteButton
                        required property var modelData
                        height: 46
                        width: Math.min(260, Math.max(150,
                                                     favoriteContent.implicitWidth + 18))
                        y: (favoriteMediaList.height - height) / 2
                        font.pixelSize: UiScale.px(11)
                        Accessible.name: modelData.kind === "bible"
                                         ? qsTr("Exibir versículo favorito %1").arg(
                                               modelData.title)
                                         : qsTr("Reproduzir favorito %1").arg(
                                               modelData.title || qsTr("Sem título"))
                        onClicked: header.activateFavorite(modelData)
                        ToolTip.visible: hovered
                        ToolTip.text: Accessible.name
                        contentItem: RowLayout {
                            id: favoriteContent
                            spacing: 7
                            MediaThumbnail {
                                visible: favoriteButton.modelData.kind === "media"
                                Layout.preferredWidth: visible ? 58 : 0
                                Layout.preferredHeight: 34
                                source: favoriteButton.modelData.thumbnailSource || ""
                                mediaType: favoriteButton.modelData.type || "audio"
                                mediaPath: favoriteButton.modelData.path || ""
                                controller: header.controller.mediaContext
                                accentColor: "#b9c7ff"
                            }
                            Rectangle {
                                visible: favoriteButton.modelData.kind === "bible"
                                Layout.preferredWidth: visible ? 42 : 0
                                Layout.preferredHeight: 34
                                radius: 4
                                color: "#30455c"
                                Label {
                                    anchors.centerIn: parent
                                    text: "▣"
                                    color: "#c7d2fe"
                                    font.pixelSize: UiScale.px(18)
                                }
                            }
                            Label {
                                Layout.fillWidth: true
                                text: favoriteButton.modelData.title || qsTr("Sem título")
                                color: "#f2f4f5"
                                elide: Text.ElideRight
                                font.pixelSize: UiScale.px(11)
                            }
                        }
                    }
                    Label {
                        anchors.centerIn: parent
                        visible: favoriteMediaList.count === 0
                        text: qsTr("Adicione mídias ou versículos aos favoritos")
                        color: "#8d979f"
                        font.pixelSize: UiScale.px(11)
                    }
                }
            }
        }
    }
}
