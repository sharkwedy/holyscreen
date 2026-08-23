pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: dashboard
    required property var controller
    signal openLibrary()
    signal openBible()
    signal openBibleBrowser()
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
    readonly property color bibleSelected: "#344d78"
    readonly property color biblePresented: "#245d45"
    readonly property color biblePresentedBorder: "#58dc9a"
    readonly property var selectedBibleTranslation: {
        for (let index = 0; index < dashboard.controller.bibleTranslations.length; ++index) {
            const translation = dashboard.controller.bibleTranslations[index]
            if (translation.id === dashboard.controller.biblePrimaryTranslationId)
                return translation
        }
        return dashboard.controller.bibleTranslations.length > 0
                ? dashboard.controller.bibleTranslations[0] : null
    }

    component PlayerButton: Button {
        id: playerButton
        implicitWidth: 48
        implicitHeight: 42
        flat: true
        font.pixelSize: 22
        font.bold: true
        contentItem: Label {
            text: playerButton.text
            color: playerButton.enabled ? "#f2f4f5" : "#626a70"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font: playerButton.font
        }
        background: Rectangle {
            radius: 6
            color: playerButton.down ? "#526173"
                  : playerButton.hovered ? "#3c4650" : "transparent"
            border.color: playerButton.highlighted ? "#9fb3ff" : "transparent"
        }
    }
    property int selectedLibraryTab: 1
    property string librarySearch: dashboard.controller.audioFileSearch
    property bool screenControlsExpanded: true
    property real mediaVolumeBeforeMute: 0.8
    property var screenBeingRenamed: null
    property var contextMediaItem: null
    property int selectedBibleBookId: 1
    property int selectedBibleChapter: 1
    property int selectedBibleVerseIndex: -1
    readonly property var bibleChapterModel: {
        const translationId = dashboard.controller.biblePrimaryTranslationId
        return translationId && selectedBibleBookId > 0
                ? dashboard.controller.bibleChapterNumbers(selectedBibleBookId) : []
    }
    readonly property var libraryModel: selectedLibraryTab === 0 ? dashboard.controller.songs
                                        : selectedLibraryTab === 1 ? dashboard.controller.folderAudioFiles
                                        : selectedLibraryTab === 2 ? dashboard.controller.folderVideoFiles
                                        : dashboard.controller.folderImageFiles

    function externalScreenCount() {
        let count = 0
        for (let index = 0; index < dashboard.controller.screens.length; ++index) {
            if (!dashboard.controller.screens[index].primary)
                ++count
        }
        return count
    }

    function ensureExternalOutputs() {
        if (dashboard.controller.outputWindows.length === 0 && externalScreenCount() > 0)
            dashboard.controller.enableAllScreens()
    }

    function bibleBook(bookId) {
        for (let index = 0; index < dashboard.controller.bibleBooks.length; ++index) {
            const book = dashboard.controller.bibleBooks[index]
            if (book.id === bookId)
                return book
        }
        return null
    }

    function searchSelectedBibleChapter() {
        const book = bibleBook(selectedBibleBookId)
        if (!book || selectedBibleChapter <= 0
                || !dashboard.controller.biblePrimaryTranslationId)
            return
        const verses = dashboard.controller.bibleVerseNumbers(selectedBibleBookId,
                                                      selectedBibleChapter)
        if (verses.length === 0)
            return
        dashboard.controller.bibleReferenceInput = book.name + " " + selectedBibleChapter
                + ":" + verses[0] + "-" + verses[verses.length - 1]
        dashboard.controller.searchBibleReference()
    }

    function selectBibleBook(bookId) {
        selectedBibleBookId = Number(bookId)
        const chapters = dashboard.controller.bibleChapterNumbers(selectedBibleBookId)
        selectedBibleChapter = chapters.length > 0 ? Number(chapters[0]) : 0
        searchSelectedBibleChapter()
    }

    function syncBibleSelectors() {
        let reference = dashboard.controller.bibleReferenceInput
        if (dashboard.controller.bibleResults.length > 0)
            reference = dashboard.controller.bibleResults[0].label
        const match = /^(.+?)\s+(\d+)\s*(?::|\.)/.exec(reference.trim())
        if (!match)
            return
        const requestedBook = match[1].toLocaleLowerCase()
        for (let index = 0; index < dashboard.controller.bibleBooks.length; ++index) {
            const book = dashboard.controller.bibleBooks[index]
            if (book.name.toLocaleLowerCase() === requestedBook) {
                selectedBibleBookId = Number(book.id)
                selectedBibleChapter = Number(match[2])
                return
            }
        }
    }

    function setScreenMediaEnabled(screen, enabled) {
        if (enabled && !screen.selected) {
            const screenId = screen.id
            if (dashboard.controller.toggleScreen(screenId, true)) {
                Qt.callLater(function() {
                    dashboard.controller.setOutputMediaEnabled(screenId, true)
                })
            }
            return
        }
        if (screen.selected)
            dashboard.controller.setOutputMediaEnabled(screen.id, enabled)
    }

    function openScreenRename(screen) {
        if (!screen.selected && !dashboard.controller.toggleScreen(screen.id, true))
            return
        screenBeingRenamed = screen
        screenNameField.text = screen.name
        screenRenameDialog.open()
        screenNameField.forceActiveFocus()
        screenNameField.selectAll()
    }

    Component.onCompleted: Qt.callLater(function() {
        ensureExternalOutputs()
        syncBibleSelectors()
        if (dashboard.controller.bibleResults.length === 0)
            searchSelectedBibleChapter()
    })

    Connections {
        target: dashboard.controller
        function onScreensChanged() { Qt.callLater(dashboard.ensureExternalOutputs) }
        function onBibleResultsChanged() {
            dashboard.selectedBibleVerseIndex = -1
            Qt.callLater(dashboard.syncBibleSelectors)
        }
    }

    Dialog {
        id: screenRenameDialog
        title: "Renomear monitor"
        modal: true
        anchors.centerIn: parent
        width: 430
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            if (dashboard.screenBeingRenamed && screenNameField.text.trim().length > 0)
                dashboard.controller.setOutputDisplayName(
                            dashboard.screenBeingRenamed.id, screenNameField.text)
        }
        contentItem: ColumnLayout {
            spacing: 10
            Label {
                text: "Nome que será exibido no HolyScreen:"
                color: dashboard.textMain
            }
            TextField {
                id: screenNameField
                Layout.fillWidth: true
                placeholderText: "Ex.: Projetor principal"
                onAccepted: screenRenameDialog.accept()
            }
            Label {
                text: "O nome técnico do monitor no Windows não será alterado."
                color: dashboard.textMuted
                font.pixelSize: 11
            }
        }
    }

    Menu {
        id: mediaContextMenu
        MenuItem {
            text: "Abrir local do arquivo"
            enabled: dashboard.contextMediaItem !== null
                     && (dashboard.contextMediaItem.path || "").length > 0
            onTriggered: dashboard.controller.openFileLocation(
                             dashboard.contextMediaItem.path)
        }
        MenuSeparator { }
        MenuItem {
            text: {
                const path = dashboard.contextMediaItem !== null
                           ? (dashboard.contextMediaItem.path || "") : ""
                const favorites = dashboard.controller.favoriteMedia
                for (let index = 0; index < favorites.length; ++index) {
                    if (favorites[index].path === path)
                        return "★ Remover dos favoritos"
                }
                return "☆ Adicionar aos favoritos"
            }
            enabled: dashboard.contextMediaItem !== null
                     && (dashboard.contextMediaItem.path || "").length > 0
            onTriggered: dashboard.controller.toggleFavoriteMedia(
                             dashboard.contextMediaItem.path)
        }
    }

    function openMediaContextMenu(item) {
        if (!item || !(item.path || "").length)
            return
        contextMediaItem = item
        mediaContextMenu.popup()
    }

    function updateSearch(value) {
        if (selectedLibraryTab === 0) dashboard.controller.songSearch = value
        else if (selectedLibraryTab === 1) dashboard.controller.audioFileSearch = value
        else if (selectedLibraryTab === 2) dashboard.controller.videoFileSearch = value
        else dashboard.controller.imageFileSearch = value
    }

    onSelectedLibraryTabChanged: updateSearch(librarySearch)

    function activateLibraryItem(item) {
        if (selectedLibraryTab === 0) dashboard.controller.selectSong(item.id)
        else dashboard.controller.addCatalogFileToPlaylist(item.path)
    }

    function activateLibraryItemFromDoubleClick(item) {
        if (selectedLibraryTab === 0) {
            dashboard.controller.selectSong(item.id)
            return
        }

        const wasPlaying = dashboard.controller.mediaState === "playing"
        let mediaId = ""
        if (item.inPlaylist) {
            for (let index = 0; index < dashboard.controller.mediaPlaylist.length; ++index) {
                if (dashboard.controller.mediaPlaylist[index].path === item.path) {
                    mediaId = dashboard.controller.mediaPlaylist[index].id
                    break
                }
            }
        } else {
            mediaId = dashboard.controller.addCatalogFileToPlaylist(item.path)
        }

        if (!wasPlaying && mediaId.length > 0)
            dashboard.controller.playMedia(mediaId)
    }

    Rectangle { anchors.fill: parent; color: dashboard.background }

    SplitView {
        anchors.fill: parent
        anchors.margins: 14
        orientation: Qt.Horizontal
        handle: Rectangle {
            implicitWidth: 14
            color: SplitHandle.pressed ? "#45515c"
                   : SplitHandle.hovered ? "#303941" : "transparent"
            Rectangle {
                anchors.centerIn: parent
                width: 2
                height: parent.height
                color: SplitHandle.pressed || SplitHandle.hovered
                       ? dashboard.accent : dashboard.line
            }
            HoverHandler { cursorShape: Qt.SplitHCursor }
        }

        Rectangle {
            SplitView.preferredWidth: Math.max(280, dashboard.width * 0.25)
            SplitView.minimumWidth: 220
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
                            placeholderTextColor: "#9ca6ad"
                            color: dashboard.textMain
                            selectionColor: dashboard.accent
                            selectedTextColor: dashboard.background
                            text: dashboard.librarySearch
                            onTextEdited: {
                                dashboard.librarySearch = text
                                dashboard.updateSearch(text)
                            }
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
                                    required property int index
                                    Layout.fillWidth: true
                                    text: modelData
                                    highlighted: index === dashboard.selectedLibraryTab
                                    onClicked: dashboard.selectedLibraryTab = index
                                }
                            }
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: dashboard.line }
                ListView {
                    id: libraryList
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true; model: dashboard.libraryModel
                    delegate: Rectangle {
                        id: catalogDelegate
                        required property var modelData
                        required property int index
                        width: ListView.view.width; height: 58
                        color: tapHandler.hovered ? "#343b43" : "transparent"
                        border.color: catalogDelegate.modelData.inPlaylist === true ? dashboard.accent : "transparent"
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 10; spacing: 10
                            Label {
                                text: dashboard.selectedLibraryTab === 0 ? "≡"
                                      : dashboard.selectedLibraryTab === 1 ? "♫"
                                      : dashboard.selectedLibraryTab === 2 ? "▶" : "▧"
                                color: dashboard.accent; font.pixelSize: 18
                            }
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 1
                                Label { Layout.fillWidth: true; text: catalogDelegate.modelData.fileName || catalogDelegate.modelData.title || "Sem título"; color: dashboard.textMain; elide: Text.ElideRight }
                                Label {
                                    Layout.fillWidth: true
                                    text: dashboard.selectedLibraryTab === 0 ? (catalogDelegate.modelData.author || "Letra") : (catalogDelegate.modelData.folderPath || "")
                                    color: dashboard.textMuted; font.pixelSize: 11; elide: Text.ElideMiddle
                                }
                            }
                            Button {
                                text: dashboard.selectedLibraryTab === 0 ? "ABRIR" : (catalogDelegate.modelData.inPlaylist ? "✓" : "+")
                                flat: true
                                enabled: dashboard.selectedLibraryTab === 0 || !catalogDelegate.modelData.inPlaylist
                                onClicked: dashboard.activateLibraryItem(catalogDelegate.modelData)
                            }
                        }
                        HoverHandler { id: tapHandler }
                        TapHandler { onDoubleTapped: dashboard.activateLibraryItemFromDoubleClick(catalogDelegate.modelData) }
                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: dashboard.openMediaContextMenu(catalogDelegate.modelData)
                        }
                    }
                    Label {
                        anchors.centerIn: parent
                        visible: libraryList.count === 0
                        text: dashboard.controller.mediaFolders.length === 0 && dashboard.selectedLibraryTab > 0
                              ? "Adicione pastas na Biblioteca para ver suas mídias"
                              : "Nenhum arquivo encontrado"
                        color: dashboard.textMuted
                    }
                }
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 42; color: dashboard.panelHigh
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 10
                        Label { text: libraryList.count + " itens"; color: dashboard.textMuted; font.pixelSize: 11 }
                        Item { Layout.fillWidth: true }
                        Button { text: "Biblioteca"; flat: true; onClicked: dashboard.openLibrary() }
                    }
                }
            }
        }

        SplitView {
            SplitView.fillWidth: true
            SplitView.minimumWidth: 360
            orientation: Qt.Vertical
            handle: Rectangle {
                implicitHeight: 14
                color: SplitHandle.pressed ? "#45515c"
                       : SplitHandle.hovered ? "#303941" : "transparent"
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width
                    height: 2
                    color: SplitHandle.pressed || SplitHandle.hovered
                           ? dashboard.accent : dashboard.line
                }
                HoverHandler { cursorShape: Qt.SplitVCursor }
            }
            Rectangle {
                SplitView.preferredHeight: Math.max(350, dashboard.height * 0.53)
                SplitView.minimumHeight: 280
                color: dashboard.panel; border.color: dashboard.line; radius: 6
                ColumnLayout {
                    anchors.fill: parent; spacing: 0
                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        color: "#000000"; radius: 6
                        SimulatedOutput {
                            anchors.fill: parent
                            controller: dashboard.controller
                            outputLabel: "PRÉVIA"
                            identifier: 1
                            wallpaper: dashboard.controller.wallpaperColor
                            wallpaperSource: dashboard.controller.wallpaperSource
                            wallpaperFit: dashboard.controller.wallpaperFit
                            showClock: dashboard.controller.clockVisible
                            clockText: dashboard.controller.clockText
                            clockPosition: dashboard.controller.clockPosition
                            clockFamily: dashboard.controller.clockFontFamily
                            clockFontSize: dashboard.controller.clockFontSize
                            clockColor: dashboard.controller.clockColor
                            clockFontBold: dashboard.controller.clockFontBold
                            clockFontItalic: dashboard.controller.clockFontItalic
                            clockBackgroundColor: dashboard.controller.clockBackgroundColor
                            clockLineHeight: dashboard.controller.clockLineHeight
                            clockCornerRadius: dashboard.controller.clockCornerRadius
                            clockTextOpacity: dashboard.controller.clockTextOpacity
                            clockBackgroundOpacity: dashboard.controller.clockBackgroundOpacity
                            clockMarginHorizontal: dashboard.controller.clockMarginHorizontal
                            clockMarginVertical: dashboard.controller.clockMarginVertical
                            clockEffect: dashboard.controller.clockEffect
                            isBlackout: dashboard.controller.blackout
                            identifyVisible: dashboard.controller.identifyVisible
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: dashboard.screenControlsExpanded ? 202 : 148
                        Layout.margins: 12; spacing: 6
                        Slider { Layout.fillWidth: true; from: 0; to: Math.max(1, dashboard.controller.mediaDurationMs); value: dashboard.controller.mediaPositionMs; onMoved: dashboard.controller.seekMedia(value) }
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: dashboard.duration(dashboard.controller.mediaPositionMs); color: dashboard.accent; font.pixelSize: 11 }
                            Item { Layout.fillWidth: true }
                            Label { text: dashboard.duration(dashboard.controller.mediaDurationMs); color: dashboard.textMuted; font.pixelSize: 11 }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            PlayerButton {
                                text: "⇄"
                                Accessible.name: "Embaralhar playlist"
                                ToolTip.visible: hovered
                                ToolTip.text: Accessible.name
                                onClicked: dashboard.controller.shuffleMediaPlaylist()
                            }
                            PlayerButton {
                                text: dashboard.controller.mediaRepeatMode === "one" ? "↻¹" : "↻"
                                highlighted: dashboard.controller.mediaRepeatMode !== "off"
                                Accessible.name: dashboard.controller.mediaRepeatMode === "off"
                                                 ? "Ativar repetição de toda a playlist"
                                                 : dashboard.controller.mediaRepeatMode === "all"
                                                   ? "Repetir somente o item atual"
                                                   : "Desativar repetição"
                                ToolTip.visible: hovered
                                ToolTip.text: Accessible.name
                                onClicked: dashboard.controller.mediaRepeatMode =
                                               dashboard.controller.mediaRepeatMode === "off" ? "all"
                                             : dashboard.controller.mediaRepeatMode === "all" ? "one" : "off"
                            }
                            Item { Layout.fillWidth: true }
                            PlayerButton { text: "⏮"; onClicked: dashboard.controller.previousMedia() }
                            PlayerButton {
                                text: dashboard.controller.mediaState === "playing"
                                      || dashboard.controller.mediaState === "buffering" ? "⏸" : "▶"
                                highlighted: true
                                onClicked: dashboard.controller.toggleMediaPause()
                            }
                            PlayerButton { text: "■"; onClicked: dashboard.controller.stopMedia() }
                            PlayerButton { text: "⏭"; onClicked: dashboard.controller.nextMedia() }
                            Item { Layout.fillWidth: true }
                            PlayerButton {
                                implicitWidth: 42
                                text: dashboard.controller.mediaVolume > 0.001 ? "🔊" : "🔇"
                                Accessible.name: dashboard.controller.mediaVolume > 0.001 ? "Mutar" : "Desmutar"
                                ToolTip.visible: hovered
                                ToolTip.text: Accessible.name
                                onClicked: {
                                    if (dashboard.controller.mediaVolume > 0.001) {
                                        dashboard.mediaVolumeBeforeMute = dashboard.controller.mediaVolume
                                        dashboard.controller.mediaVolume = 0
                                    } else {
                                        dashboard.controller.mediaVolume = Math.max(0.05,
                                                                          dashboard.mediaVolumeBeforeMute)
                                    }
                                }
                            }
                            Slider { Layout.preferredWidth: 110; from: 0; to: 1; value: dashboard.controller.mediaVolume; onMoved: dashboard.controller.mediaVolume = value }
                            PlayerButton {
                                implicitWidth: 42
                                text: dashboard.screenControlsExpanded ? "▾" : "▸"
                                Accessible.name: dashboard.screenControlsExpanded
                                                 ? "Ocultar seleção de telas"
                                                 : "Mostrar seleção de telas"
                                ToolTip.visible: hovered
                                ToolTip.text: Accessible.name
                                onClicked: dashboard.screenControlsExpanded = !dashboard.screenControlsExpanded
                            }
                        }
                        Rectangle {
                            visible: dashboard.screenControlsExpanded
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            radius: 6
                            color: "#2b3137"
                            border.color: dashboard.line
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 8
                                spacing: 12
                                Label {
                                    text: "Exibir vídeo em:"
                                    color: dashboard.textMain
                                    font.bold: true
                                    font.pixelSize: 12
                                }
                                Repeater {
                                    model: dashboard.controller.screens
                                    delegate: CheckBox {
                                        id: screenCheckBox
                                        required property var modelData
                                        visible: !screenCheckBox.modelData.primary
                                        text: screenCheckBox.modelData.name
                                        checked: screenCheckBox.modelData.selected && screenCheckBox.modelData.mediaEnabled
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
                                                font.pixelSize: 14
                                            }
                                        }
                                        contentItem: Label {
                                            leftPadding: screenCheckBox.indicator.width + screenCheckBox.spacing
                                            text: screenCheckBox.text
                                            color: dashboard.textMain
                                            font.pixelSize: 12
                                            verticalAlignment: Text.AlignVCenter
                                            elide: Text.ElideRight
                                        }
                                        ToolTip.visible: hovered
                                        ToolTip.text: "Clique com o botão direito para renomear"
                                        onClicked: dashboard.setScreenMediaEnabled(screenCheckBox.modelData, checked)
                                        TapHandler {
                                            acceptedButtons: Qt.RightButton
                                            onTapped: dashboard.openScreenRename(screenCheckBox.modelData)
                                        }
                                    }
                                }
                                Label {
                                    visible: dashboard.externalScreenCount() === 0
                                    text: "nenhuma tela externa detectada"
                                    color: dashboard.textMuted
                                    font.pixelSize: 12
                                }
                                Button {
                                    visible: dashboard.externalScreenCount() > dashboard.controller.outputWindows.length
                                    text: "Ativar todas"
                                    onClicked: dashboard.controller.enableAllScreens()
                                }
                                Item { Layout.fillWidth: true }
                            }
                        }
                    }
                }
            }
            Rectangle {
                SplitView.fillHeight: true
                SplitView.minimumHeight: 120
                color: dashboard.panel; border.color: dashboard.line; radius: 6
                ColumnLayout {
                    anchors.fill: parent; spacing: 0
                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 42; color: dashboard.panelHigh
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 10
                            Label { text: "▾  Reprodução"; color: dashboard.textMain; font.bold: true; font.pixelSize: 12 }
                            Item { Layout.fillWidth: true }
                            Button { text: "Salvar"; flat: true; onClicked: savePlaylistDialog.open() }
                            Button { text: "Limpar"; flat: true; onClicked: dashboard.controller.clearMediaPlaylist() }
                        }
                    }
                    ListView {
                        id: playlistList
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        spacing: 2
                        model: dashboard.controller.mediaPlaylist
                        delegate: Rectangle {
                            id: playlistDelegate
                            required property var modelData
                            required property int index
                            property real dragDistance: 0
                            width: ListView.view.width; height: 48
                            z: reorderDrag.active ? 10 : 0
                            radius: 4
                            color: reorderDrag.active ? "#40546b"
                                  : dashboard.controller.currentMediaId === playlistDelegate.modelData.id ? "#263b55"
                                  : playlistTap.hovered ? "#30373d"
                                  : (index % 2 ? "#1d2226" : "transparent")
                            border.color: reorderDrag.active ? dashboard.accent : "transparent"
                            transform: Translate { y: playlistDelegate.dragDistance }
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
                                Label { text: "☰"; color: dashboard.accent; font.pixelSize: 18; Layout.preferredWidth: 24 }
                                Label { text: playlistDelegate.index + 1; color: dashboard.textMuted; Layout.preferredWidth: 24 }
                                Label { Layout.fillWidth: true; text: playlistDelegate.modelData.title; color: dashboard.textMain; elide: Text.ElideRight }
                                Label { text: dashboard.duration(playlistDelegate.modelData.durationMs); color: dashboard.textMuted; font.pixelSize: 11 }
                                PlayerButton {
                                    implicitWidth: 38
                                    implicitHeight: 36
                                    font.pixelSize: 18
                                    text: "▶"
                                    onClicked: dashboard.controller.playMedia(playlistDelegate.modelData.id)
                                }
                            }
                            HoverHandler { id: playlistTap }
                            TapHandler {
                                acceptedButtons: Qt.LeftButton
                                onDoubleTapped: dashboard.controller.playMedia(playlistDelegate.modelData.id)
                            }
                            TapHandler {
                                acceptedButtons: Qt.RightButton
                                onTapped: dashboard.openMediaContextMenu(playlistDelegate.modelData)
                            }
                            DragHandler {
                                id: reorderDrag
                                target: null
                                xAxis.enabled: false
                                onTranslationChanged: {
                                    if (active)
                                        playlistDelegate.dragDistance = translation.y
                                }
                                onActiveChanged: {
                                    if (active)
                                        return
                                    const mediaId = playlistDelegate.modelData.id
                                    const sourceIndex = playlistDelegate.index
                                    const offset = Math.round(playlistDelegate.dragDistance
                                                              / playlistDelegate.height)
                                    const targetIndex = Math.max(0, Math.min(playlistList.count - 1,
                                                                           sourceIndex + offset))
                                    persistentTranslation = Qt.vector2d(0, 0)
                                    playlistDelegate.dragDistance = 0
                                    if (targetIndex !== sourceIndex)
                                        dashboard.controller.moveMedia(mediaId, targetIndex)
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            SplitView.preferredWidth: Math.max(300, dashboard.width * 0.27)
            SplitView.minimumWidth: 280
            color: dashboard.panel; border.color: dashboard.line; radius: 6
            ColumnLayout {
                anchors.fill: parent; spacing: 0
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 48; color: dashboard.panelHigh
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 10
                        Label { text: "▣  Bíblia Sagrada"; color: dashboard.textMain; font.bold: true }
                        Item { Layout.fillWidth: true }
                        Button { text: "NAVEGAR"; flat: true; onClicked: dashboard.openBibleBrowser() }
                        Button { text: "☰"; flat: true; onClicked: dashboard.openBible() }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true; Layout.margins: 8
                    ComboBox {
                        id: bibleBookCombo
                        Layout.fillWidth: true
                        model: dashboard.controller.bibleBooks
                        textRole: "name"
                        valueRole: "id"
                        currentIndex: {
                            for (let index = 0; index < dashboard.controller.bibleBooks.length; ++index) {
                                if (dashboard.controller.bibleBooks[index].id
                                        === dashboard.selectedBibleBookId)
                                    return index
                            }
                            return -1
                        }
                        onActivated: dashboard.selectBibleBook(currentValue)
                    }
                    ComboBox {
                        id: bibleChapterCombo
                        Layout.preferredWidth: 76
                        model: dashboard.bibleChapterModel
                        currentIndex: {
                            for (let index = 0; index < dashboard.bibleChapterModel.length; ++index) {
                                if (Number(dashboard.bibleChapterModel[index])
                                        === dashboard.selectedBibleChapter)
                                    return index
                            }
                            return dashboard.bibleChapterModel.length > 0 ? 0 : -1
                        }
                        onActivated: {
                            dashboard.selectedBibleChapter = Number(currentValue)
                            dashboard.searchSelectedBibleChapter()
                        }
                    }
                    ComboBox {
                        id: bibleTranslationCombo
                        Layout.preferredWidth: 110
                        model: dashboard.controller.bibleTranslations
                        textRole: "abbreviation"
                        valueRole: "id"
                        currentIndex: {
                            for (let index = 0; index < dashboard.controller.bibleTranslations.length; ++index) {
                                if (dashboard.controller.bibleTranslations[index].id
                                        === dashboard.controller.biblePrimaryTranslationId)
                                    return index
                            }
                            return dashboard.controller.bibleTranslations.length > 0 ? 0 : -1
                        }
                        onActivated: {
                            dashboard.controller.biblePrimaryTranslationId = currentValue
                            dashboard.searchSelectedBibleChapter()
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: dashboard.line }
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 0
                    model: dashboard.controller.bibleResults
                    delegate: Rectangle {
                        id: bibleVerseDelegate
                        required property var modelData
                        required property int index
                        readonly property bool isSelected: dashboard.selectedBibleVerseIndex === index
                        readonly property bool isPresented:
                            dashboard.controller.currentPresentationType === "bible"
                            && dashboard.controller.currentSlideLabel === bibleVerseDelegate.modelData.label
                        width: ListView.view.width
                        height: verseText.implicitHeight + 30
                        color: isPresented
                               ? dashboard.biblePresented
                               : isSelected ? dashboard.bibleSelected
                               : index % 2 ? dashboard.panelHigh : "transparent"
                        border.color: isPresented
                                      ? dashboard.biblePresentedBorder
                                      : isSelected ? dashboard.accent : "transparent"
                        border.width: isPresented || isSelected ? 1 : 0
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 10; spacing: 10
                            Label { text: bibleVerseDelegate.modelData.verse; color: dashboard.accent; font.bold: true; Layout.alignment: Qt.AlignTop }
                            Label {
                                id: verseText
                                Layout.fillWidth: true
                                text: bibleVerseDelegate.modelData.versions
                                      && bibleVerseDelegate.modelData.versions.length > 0
                                      ? bibleVerseDelegate.modelData.versions[0].text
                                      : bibleVerseDelegate.modelData.text
                                color: dashboard.textMain
                                wrapMode: Text.WordWrap
                                lineHeight: 1.25
                            }
                        }
                        TapHandler {
                            onTapped: dashboard.selectedBibleVerseIndex = bibleVerseDelegate.index
                            onDoubleTapped: {
                                dashboard.selectedBibleVerseIndex = bibleVerseDelegate.index
                                dashboard.controller.showBibleVerse(bibleVerseDelegate.index)
                            }
                        }
                    }
                    Label {
                        anchors.centerIn: parent
                        visible: parent.count === 0
                        text: dashboard.controller.bibleTranslations.length === 0
                              ? "Importe uma tradução bíblica para visualizar passagens"
                              : "Nenhum versículo encontrado"
                        color: dashboard.textMuted
                    }
                }
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 48; color: dashboard.panelHigh
                    ColumnLayout {
                        anchors.centerIn: parent; spacing: 1
                        Label {
                            text: dashboard.selectedBibleTranslation
                                  ? dashboard.selectedBibleTranslation.displayName
                                  : "Nenhuma tradução importada"
                            color: dashboard.textMain
                            font.bold: true
                            font.pixelSize: 10
                        }
                        Label {
                            text: dashboard.selectedBibleTranslation
                                  ? (dashboard.selectedBibleTranslation.license === "public-domain"
                                     ? "Domínio público"
                                     : dashboard.selectedBibleTranslation.license)
                                  : "Use uma origem com licença adequada"
                            color: dashboard.textMuted
                            font.pixelSize: 9
                        }
                    }
                }
            }
        }
    }

    FileDialog {
        id: savePlaylistDialog
        title: "Salvar playlist"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Playlist M3U8 (*.m3u8)"]
        defaultSuffix: "m3u8"
        onAccepted: dashboard.controller.saveMediaPlaylist(selectedFile)
    }

    function duration(milliseconds) {
        const totalSeconds = Math.max(0, Math.floor((milliseconds || 0) / 1000))
        const minutes = Math.floor(totalSeconds / 60)
        const seconds = totalSeconds % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }
}
