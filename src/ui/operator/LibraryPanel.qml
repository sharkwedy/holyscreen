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
    signal editOnlineLyrics(string key)

    property int selectedTab: 1
    property string searchText: panel.controller.audioFileSearch
    readonly property var libraryModel: panel.selectedTab === 0
                                                ? panel.lyricsModel()
                                      : panel.selectedTab === 1
                                                ? panel.controller.folderAudioFiles
                                      : panel.selectedTab === 2
                                                ? panel.controller.folderVideoFiles
                                                : panel.controller.folderImageFiles

    color: panel.panelColor
    border.color: panel.lineColor
    radius: 6

    function lyricsModel() {
        const combined = []
        const localSongs = panel.controller.songs
        for (let index = 0; index < localSongs.length; ++index) {
            const song = localSongs[index]
            combined.push({"kind": "local", "id": song.id, "title": song.title,
                           "author": song.author, "slideCount": song.slideCount,
                           "sequenceCount": song.sequenceCount})
        }
        if (panel.searchText.trim().length >= 3) {
            combined.push({"kind": "onlineHeader"})
            const online = panel.controller.onlineLyricsResults
            for (let onlineIndex = 0; onlineIndex < online.length; ++onlineIndex) {
                const result = online[onlineIndex]
                combined.push({"kind": "online", "key": result.key,
                               "title": result.title, "author": result.artist,
                               "provider": result.provider, "album": result.album,
                               "savedCount": result.savedCount || 0})
            }
            if (panel.controller.onlineLyricsError.length > 0)
                combined.push({"kind": "onlineError",
                               "message": panel.controller.onlineLyricsError})
            else if (panel.controller.onlineLyricsStatus.length > 0)
                combined.push({"kind": "onlineStatus",
                               "message": panel.controller.onlineLyricsStatus})
        }
        return combined
    }

    function scheduleOnlineSearch() {
        if (panel.selectedTab === 0 && panel.visible
                && panel.searchText.trim().length >= 3) {
            onlineSearchDebounce.restart()
        } else {
            onlineSearchDebounce.stop()
            panel.controller.cancelOnlineLyricsSearch()
        }
    }

    function updateSearch(value) {
        if (panel.selectedTab === 0) panel.controller.songSearch = value
        else if (panel.selectedTab === 1) panel.controller.audioFileSearch = value
        else if (panel.selectedTab === 2) panel.controller.videoFileSearch = value
        else panel.controller.imageFileSearch = value
        panel.scheduleOnlineSearch()
    }

    function activateItem(item) {
        if (item.kind === "online") panel.controller.saveOnlineLyrics(item.key)
        else if (panel.selectedTab === 0) panel.controller.selectSong(item.id)
        else panel.controller.addCatalogFileToPlaylist(item.path)
    }

    function activateItemFromDoubleClick(item) {
        if (item.kind === "online") {
            panel.editOnlineLyrics(item.key)
            return
        }
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
    onVisibleChanged: scheduleOnlineSearch()

    Timer {
        id: onlineSearchDebounce
        interval: 500
        repeat: false
        onTriggered: panel.controller.searchOnlineLyrics(panel.searchText)
    }

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
            objectName: "libraryList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: panel.libraryModel
            delegate: Rectangle {
                id: catalogDelegate
                required property var modelData
                required property int index
                width: ListView.view.width
                readonly property bool isHeader: catalogDelegate.modelData.kind === "onlineHeader"
                readonly property bool isError: catalogDelegate.modelData.kind === "onlineError"
                readonly property bool isStatus: catalogDelegate.modelData.kind === "onlineStatus"
                readonly property bool isOnline: catalogDelegate.modelData.kind === "online"
                height: isHeader || isError || isStatus ? 42 : 64
                color: catalogHover.hovered ? "#343b43" : "transparent"
                border.color: catalogDelegate.modelData.inPlaylist === true
                              ? panel.accentColor : "transparent"
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10
                    visible: !catalogDelegate.isHeader && !catalogDelegate.isError
                             && !catalogDelegate.isStatus
                    MediaThumbnail {
                        objectName: "libraryThumbnail-" + catalogDelegate.index
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 38
                        source: catalogDelegate.modelData.thumbnailSource || ""
                        mediaType: panel.selectedTab === 0
                                   ? "lyrics"
                                   : (catalogDelegate.modelData.type || "audio")
                        mediaPath: catalogDelegate.modelData.path || ""
                        controller: panel.controller
                        accentColor: panel.accentColor
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
                            text: catalogDelegate.isOnline
                                  ? qsTr("%1 · %2 · resultado online")
                                        .arg(catalogDelegate.modelData.author)
                                        .arg(catalogDelegate.modelData.provider)
                                  : panel.selectedTab === 0
                                  ? (catalogDelegate.modelData.author || qsTr("Letra"))
                                  : (catalogDelegate.modelData.folderPath || "")
                            color: panel.textMutedColor
                            font.pixelSize: UiScale.px(11)
                            elide: Text.ElideMiddle
                        }
                    }
                    Button {
                        text: catalogDelegate.isOnline
                              ? (catalogDelegate.modelData.savedCount > 0
                                 ? qsTr("SALVA ✓") : qsTr("SALVAR"))
                              : panel.selectedTab === 0
                              ? qsTr("ABRIR")
                              : (catalogDelegate.modelData.inPlaylist ? "✓" : "+")
                        flat: true
                        enabled: (!catalogDelegate.isOnline
                                  || !panel.controller.onlineLyricsBusy)
                                 && (panel.selectedTab === 0
                                     || !catalogDelegate.modelData.inPlaylist)
                        onClicked: panel.activateItem(catalogDelegate.modelData)
                    }
                    Button {
                        visible: catalogDelegate.isOnline
                        text: qsTr("EDITAR")
                        enabled: !panel.controller.onlineLyricsBusy
                        onClicked: panel.editOnlineLyrics(catalogDelegate.modelData.key)
                    }
                }
                Label {
                    anchors.fill: parent
                    anchors.margins: 10
                    visible: catalogDelegate.isHeader
                    text: panel.controller.onlineLyricsBusy
                          ? qsTr("Buscando letras online…")
                          : qsTr("RESULTADOS ONLINE (%1)").arg(
                                panel.controller.onlineLyricsResults.length)
                    color: panel.accentColor
                    font.bold: true
                    verticalAlignment: Text.AlignVCenter
                }
                Label {
                    anchors.fill: parent
                    anchors.margins: 10
                    visible: catalogDelegate.isError || catalogDelegate.isStatus
                    text: catalogDelegate.modelData.message || ""
                    color: catalogDelegate.isError ? "#ffba70" : "#70e1a7"
                    wrapMode: Text.Wrap
                    verticalAlignment: Text.AlignVCenter
                }
                HoverHandler { id: catalogHover }
                TapHandler {
                    enabled: !catalogDelegate.isHeader && !catalogDelegate.isError
                             && !catalogDelegate.isStatus
                    onDoubleTapped: panel.activateItemFromDoubleClick(
                                        catalogDelegate.modelData)
                }
                TapHandler {
                    enabled: !catalogDelegate.isHeader && !catalogDelegate.isError
                             && !catalogDelegate.isStatus
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
                    text: panel.selectedTab === 0
                          ? qsTr("%1 locais · %2 online").arg(
                                panel.controller.songs.length).arg(
                                panel.controller.onlineLyricsResults.length)
                          : qsTr("%1 itens").arg(libraryList.count)
                    color: panel.textMutedColor
                    font.pixelSize: UiScale.px(11)
                }
                Item { Layout.fillWidth: true }
                Button {
                    visible: panel.selectedTab === 0
                    text: qsTr("BUSCAR ONLINE")
                    flat: true
                    onClicked: panel.editOnlineLyrics("")
                }
                Button {
                    text: qsTr("Biblioteca")
                    flat: true
                    onClicked: panel.openLibrary()
                }
            }
        }
    }
}
