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
        for (let index = 0; index < controller.bibleTranslations.length; ++index) {
            const translation = controller.bibleTranslations[index]
            if (translation.id === controller.biblePrimaryTranslationId)
                return translation
        }
        return controller.bibleTranslations.length > 0
                ? controller.bibleTranslations[0] : null
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
    property string librarySearch: controller.audioFileSearch
    property bool screenControlsExpanded: true
    property real mediaVolumeBeforeMute: 0.8
    property var screenBeingRenamed: null
    property var contextMediaItem: null
    property int selectedBibleBookId: 1
    property int selectedBibleChapter: 1
    property int selectedBibleVerseIndex: -1
    readonly property var bibleChapterModel: {
        const translationId = controller.biblePrimaryTranslationId
        return translationId && selectedBibleBookId > 0
                ? controller.bibleChapterNumbers(selectedBibleBookId) : []
    }
    readonly property var libraryModel: selectedLibraryTab === 0 ? controller.songs
                                        : selectedLibraryTab === 1 ? controller.folderAudioFiles
                                        : selectedLibraryTab === 2 ? controller.folderVideoFiles
                                        : controller.folderImageFiles

    function externalScreenCount() {
        let count = 0
        for (let index = 0; index < controller.screens.length; ++index) {
            if (!controller.screens[index].primary)
                ++count
        }
        return count
    }

    function ensureExternalOutputs() {
        if (controller.outputWindows.length === 0 && externalScreenCount() > 0)
            controller.enableAllScreens()
    }

    function bibleBook(bookId) {
        for (let index = 0; index < controller.bibleBooks.length; ++index) {
            const book = controller.bibleBooks[index]
            if (book.id === bookId)
                return book
        }
        return null
    }

    function searchSelectedBibleChapter() {
        const book = bibleBook(selectedBibleBookId)
        if (!book || selectedBibleChapter <= 0
                || !controller.biblePrimaryTranslationId)
            return
        const verses = controller.bibleVerseNumbers(selectedBibleBookId,
                                                      selectedBibleChapter)
        if (verses.length === 0)
            return
        controller.bibleReferenceInput = book.name + " " + selectedBibleChapter
                + ":" + verses[0] + "-" + verses[verses.length - 1]
        controller.searchBibleReference()
    }

    function selectBibleBook(bookId) {
        selectedBibleBookId = Number(bookId)
        const chapters = controller.bibleChapterNumbers(selectedBibleBookId)
        selectedBibleChapter = chapters.length > 0 ? Number(chapters[0]) : 0
        searchSelectedBibleChapter()
    }

    function syncBibleSelectors() {
        let reference = controller.bibleReferenceInput
        if (controller.bibleResults.length > 0)
            reference = controller.bibleResults[0].label
        const match = /^(.+?)\s+(\d+)\s*(?::|\.)/.exec(reference.trim())
        if (!match)
            return
        const requestedBook = match[1].toLocaleLowerCase()
        for (let index = 0; index < controller.bibleBooks.length; ++index) {
            const book = controller.bibleBooks[index]
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
            if (controller.toggleScreen(screenId, true)) {
                Qt.callLater(function() {
                    controller.setOutputMediaEnabled(screenId, true)
                })
            }
            return
        }
        if (screen.selected)
            controller.setOutputMediaEnabled(screen.id, enabled)
    }

    function openScreenRename(screen) {
        if (!screen.selected && !controller.toggleScreen(screen.id, true))
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
        if (controller.bibleResults.length === 0)
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
        if (selectedLibraryTab === 0) controller.songSearch = value
        else if (selectedLibraryTab === 1) controller.audioFileSearch = value
        else if (selectedLibraryTab === 2) controller.videoFileSearch = value
        else controller.imageFileSearch = value
    }

    onSelectedLibraryTabChanged: updateSearch(librarySearch)

    function activateLibraryItem(item) {
        if (selectedLibraryTab === 0) controller.selectSong(item.id)
        else controller.addCatalogFileToPlaylist(item.path)
    }

    function activateLibraryItemFromDoubleClick(item) {
        if (selectedLibraryTab === 0) {
            controller.selectSong(item.id)
            return
        }

        const wasPlaying = controller.mediaState === "playing"
        let mediaId = ""
        if (item.inPlaylist) {
            for (let index = 0; index < controller.mediaPlaylist.length; ++index) {
                if (controller.mediaPlaylist[index].path === item.path) {
                    mediaId = controller.mediaPlaylist[index].id
                    break
                }
            }
        } else {
            mediaId = controller.addCatalogFileToPlaylist(item.path)
        }

        if (!wasPlaying && mediaId.length > 0)
            controller.playMedia(mediaId)
    }

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
                        border.color: modelData.inPlaylist === true ? dashboard.accent : "transparent"
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
                                Label { Layout.fillWidth: true; text: modelData.fileName || modelData.title || "Sem título"; color: dashboard.textMain; elide: Text.ElideRight }
                                Label {
                                    Layout.fillWidth: true
                                    text: dashboard.selectedLibraryTab === 0 ? (modelData.author || "Letra") : (modelData.folderPath || "")
                                    color: dashboard.textMuted; font.pixelSize: 11; elide: Text.ElideMiddle
                                }
                            }
                            Button {
                                text: dashboard.selectedLibraryTab === 0 ? "ABRIR" : (modelData.inPlaylist ? "✓" : "+")
                                flat: true
                                enabled: dashboard.selectedLibraryTab === 0 || !modelData.inPlaylist
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
                            clockFontSize: controller.clockFontSize
                            clockColor: controller.clockColor
                            clockFontBold: controller.clockFontBold
                            clockFontItalic: controller.clockFontItalic
                            clockBackgroundColor: controller.clockBackgroundColor
                            clockLineHeight: controller.clockLineHeight
                            clockCornerRadius: controller.clockCornerRadius
                            clockTextOpacity: controller.clockTextOpacity
                            clockBackgroundOpacity: controller.clockBackgroundOpacity
                            clockMarginHorizontal: controller.clockMarginHorizontal
                            clockMarginVertical: controller.clockMarginVertical
                            clockEffect: controller.clockEffect
                            isBlackout: controller.blackout
                            identifyVisible: controller.identifyVisible
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: dashboard.screenControlsExpanded ? 202 : 148
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
                            PlayerButton {
                                text: "⇄"
                                Accessible.name: "Embaralhar playlist"
                                ToolTip.visible: hovered
                                ToolTip.text: Accessible.name
                                onClicked: controller.shuffleMediaPlaylist()
                            }
                            PlayerButton {
                                text: controller.mediaRepeatMode === "one" ? "↻¹" : "↻"
                                highlighted: controller.mediaRepeatMode !== "off"
                                Accessible.name: controller.mediaRepeatMode === "off"
                                                 ? "Ativar repetição de toda a playlist"
                                                 : controller.mediaRepeatMode === "all"
                                                   ? "Repetir somente o item atual"
                                                   : "Desativar repetição"
                                ToolTip.visible: hovered
                                ToolTip.text: Accessible.name
                                onClicked: controller.mediaRepeatMode =
                                               controller.mediaRepeatMode === "off" ? "all"
                                             : controller.mediaRepeatMode === "all" ? "one" : "off"
                            }
                            Item { Layout.fillWidth: true }
                            PlayerButton { text: "⏮"; onClicked: controller.previousMedia() }
                            PlayerButton {
                                text: controller.mediaState === "playing"
                                      || controller.mediaState === "buffering" ? "⏸" : "▶"
                                highlighted: true
                                onClicked: controller.toggleMediaPause()
                            }
                            PlayerButton { text: "■"; onClicked: controller.stopMedia() }
                            PlayerButton { text: "⏭"; onClicked: controller.nextMedia() }
                            Item { Layout.fillWidth: true }
                            PlayerButton {
                                implicitWidth: 42
                                text: controller.mediaVolume > 0.001 ? "🔊" : "🔇"
                                Accessible.name: controller.mediaVolume > 0.001 ? "Mutar" : "Desmutar"
                                ToolTip.visible: hovered
                                ToolTip.text: Accessible.name
                                onClicked: {
                                    if (controller.mediaVolume > 0.001) {
                                        dashboard.mediaVolumeBeforeMute = controller.mediaVolume
                                        controller.mediaVolume = 0
                                    } else {
                                        controller.mediaVolume = Math.max(0.05,
                                                                          dashboard.mediaVolumeBeforeMute)
                                    }
                                }
                            }
                            Slider { Layout.preferredWidth: 110; from: 0; to: 1; value: controller.mediaVolume; onMoved: controller.mediaVolume = value }
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
                                    model: controller.screens
                                    delegate: CheckBox {
                                        id: screenCheckBox
                                        required property var modelData
                                        visible: !modelData.primary
                                        text: modelData.name
                                        checked: modelData.selected && modelData.mediaEnabled
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
                                        onClicked: dashboard.setScreenMediaEnabled(modelData, checked)
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
                                    visible: dashboard.externalScreenCount() > controller.outputWindows.length
                                    text: "Ativar todas"
                                    onClicked: controller.enableAllScreens()
                                }
                                Item { Layout.fillWidth: true }
                            }
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
                            Button { text: "Salvar"; flat: true; onClicked: savePlaylistDialog.open() }
                            Button { text: "Limpar"; flat: true; onClicked: controller.clearMediaPlaylist() }
                        }
                    }
                    ListView {
                        id: playlistList
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        spacing: 2
                        model: controller.mediaPlaylist
                        delegate: Rectangle {
                            id: playlistDelegate
                            required property var modelData
                            required property int index
                            property real dragDistance: 0
                            width: ListView.view.width; height: 48
                            z: reorderDrag.active ? 10 : 0
                            radius: 4
                            color: reorderDrag.active ? "#40546b"
                                  : controller.currentMediaId === modelData.id ? "#263b55"
                                  : playlistTap.hovered ? "#30373d"
                                  : (index % 2 ? "#1d2226" : "transparent")
                            border.color: reorderDrag.active ? dashboard.accent : "transparent"
                            transform: Translate { y: playlistDelegate.dragDistance }
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
                                Label { text: "☰"; color: dashboard.accent; font.pixelSize: 18; Layout.preferredWidth: 24 }
                                Label { text: index + 1; color: dashboard.textMuted; Layout.preferredWidth: 24 }
                                Label { Layout.fillWidth: true; text: modelData.title; color: dashboard.textMain; elide: Text.ElideRight }
                                Label { text: dashboard.duration(modelData.durationMs); color: dashboard.textMuted; font.pixelSize: 11 }
                                PlayerButton {
                                    implicitWidth: 38
                                    implicitHeight: 36
                                    font.pixelSize: 18
                                    text: "▶"
                                    onClicked: controller.playMedia(playlistDelegate.modelData.id)
                                }
                            }
                            HoverHandler { id: playlistTap }
                            TapHandler {
                                acceptedButtons: Qt.LeftButton
                                onDoubleTapped: controller.playMedia(playlistDelegate.modelData.id)
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
                                        controller.moveMedia(mediaId, targetIndex)
                                }
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
                            && dashboard.controller.currentSlideLabel === modelData.label
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
        onAccepted: controller.saveMediaPlaylist(selectedFile)
    }

    function duration(milliseconds) {
        const totalSeconds = Math.max(0, Math.floor((milliseconds || 0) / 1000))
        const minutes = Math.floor(totalSeconds / 60)
        const seconds = totalSeconds % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }
}
