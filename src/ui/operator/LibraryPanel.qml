pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: panel

    required property var controller
    required property color backgroundColor
    required property color panelColor
    required property color panelHighColor
    required property color lineColor
    required property color textMainColor
    required property color textMutedColor
    required property color accentColor

    signal openLibrary()
    signal showMediaOptions(var item)

    property int selectedTab: 1
    property string searchText: panel.controller.audioFileSearch
    readonly property var libraryModel: panel.selectedTab === 0
                                                ? panel.controller.songs
                                      : panel.selectedTab === 1
                                                ? panel.controller.folderAudioFiles
                                      : panel.selectedTab === 2
                                                ? panel.controller.folderVideoFiles
                                                : panel.controller.folderImageFiles

    color: panel.panelColor
    border.color: panel.lineColor
    radius: 6

    function updateSearch(value) {
        if (panel.selectedTab === 0) panel.controller.songSearch = value
        else if (panel.selectedTab === 1) panel.controller.audioFileSearch = value
        else if (panel.selectedTab === 2) panel.controller.videoFileSearch = value
        else panel.controller.imageFileSearch = value
    }

    function activateItem(item) {
        if (panel.selectedTab === 0) panel.controller.selectSong(item.id)
        else panel.controller.addCatalogFileToPlaylist(item.path)
    }

    function activateItemFromDoubleClick(item) {
        if (panel.selectedTab === 0) {
            panel.controller.selectSong(item.id)
            return
        }

        const wasPlaying = panel.controller.mediaState === "playing"
        let mediaId = ""
        if (item.inPlaylist) {
            for (let index = 0; index < panel.controller.mediaPlaylist.length; ++index) {
                if (panel.controller.mediaPlaylist[index].path === item.path) {
                    mediaId = panel.controller.mediaPlaylist[index].id
                    break
                }
            }
        } else {
            mediaId = panel.controller.addCatalogFileToPlaylist(item.path)
        }

        if (!wasPlaying && mediaId.length > 0)
            panel.controller.playMedia(mediaId)
    }

    onSelectedTabChanged: updateSearch(panel.searchText)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 92
            color: "transparent"
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8
                TextField {
                    id: mediaSearch
                    Layout.fillWidth: true
                    placeholderText: qsTr("Pesquisar mídia...")
                    placeholderTextColor: "#9ca6ad"
                    color: panel.textMainColor
                    selectionColor: panel.accentColor
                    selectedTextColor: panel.backgroundColor
                    Accessible.name: qsTr("Pesquisar mídia")
                    text: panel.searchText
                    onTextEdited: {
                        panel.searchText = text
                        panel.updateSearch(text)
                    }
                    leftPadding: 34
                    background: Rectangle {
                        color: panel.panelHighColor
                        border.color: mediaSearch.activeFocus
                                      ? panel.accentColor : panel.lineColor
                        border.width: mediaSearch.activeFocus ? 2 : 1
                        radius: 4
                    }
                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: "⌕"
                        color: panel.textMutedColor
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    Repeater {
                        model: [qsTr("Letras"), qsTr("Áudio"),
                                qsTr("Vídeo"), qsTr("Imagem")]
                        Button {
                            required property string modelData
                            required property int index
                            Layout.fillWidth: true
                            text: modelData
                            highlighted: index === panel.selectedTab
                            onClicked: panel.selectedTab = index
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: panel.lineColor
        }

        ListView {
            id: libraryList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: panel.libraryModel
            delegate: Rectangle {
                id: catalogDelegate
                required property var modelData
                required property int index
                width: ListView.view.width
                height: 58
                color: catalogHover.hovered ? "#343b43" : "transparent"
                border.color: catalogDelegate.modelData.inPlaylist === true
                              ? panel.accentColor : "transparent"
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10
                    Label {
                        text: panel.selectedTab === 0 ? "≡"
                              : panel.selectedTab === 1 ? "♫"
                              : panel.selectedTab === 2 ? "▶" : "▧"
                        color: panel.accentColor
                        font.pixelSize: 18
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Label {
                            Layout.fillWidth: true
                            text: catalogDelegate.modelData.fileName
                                  || catalogDelegate.modelData.title
                                  || qsTr("Sem título")
                            color: panel.textMainColor
                            elide: Text.ElideRight
                        }
                        Label {
                            Layout.fillWidth: true
                            text: panel.selectedTab === 0
                                  ? (catalogDelegate.modelData.author || qsTr("Letra"))
                                  : (catalogDelegate.modelData.folderPath || "")
                            color: panel.textMutedColor
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                        }
                    }
                    Button {
                        text: panel.selectedTab === 0
                              ? qsTr("ABRIR")
                              : (catalogDelegate.modelData.inPlaylist ? "✓" : "+")
                        flat: true
                        enabled: panel.selectedTab === 0
                                 || !catalogDelegate.modelData.inPlaylist
                        onClicked: panel.activateItem(catalogDelegate.modelData)
                    }
                }
                HoverHandler { id: catalogHover }
                TapHandler {
                    onDoubleTapped: panel.activateItemFromDoubleClick(
                                        catalogDelegate.modelData)
                }
                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: panel.showMediaOptions(catalogDelegate.modelData)
                }
            }
            Label {
                anchors.centerIn: parent
                visible: libraryList.count === 0
                text: panel.controller.mediaFolders.length === 0 && panel.selectedTab > 0
                      ? qsTr("Adicione pastas na Biblioteca para ver suas mídias")
                      : qsTr("Nenhum arquivo encontrado")
                color: panel.textMutedColor
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            color: panel.panelHighColor
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                Label {
                    text: qsTr("%1 itens").arg(libraryList.count)
                    color: panel.textMutedColor
                    font.pixelSize: 11
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("Biblioteca")
                    flat: true
                    onClicked: panel.openLibrary()
                }
            }
        }
    }
}
