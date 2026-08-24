pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: dashboard
    required property var controller
    property var layoutSettings: null
    signal openLibrary()
    signal openBible()
    signal openBibleBrowser()
    signal importAudio()
    signal importVideo()
    signal importImage()

    function saveLayout() {
        if (!layoutSettings) return
        layoutSettings.dashboardHorizontalState = horizontalSplit.saveState()
        layoutSettings.dashboardVerticalState = centerSplit.saveState()
    }

    function restoreLayout() {
        if (!layoutSettings) return
        if (layoutSettings.dashboardHorizontalState)
            horizontalSplit.restoreState(layoutSettings.dashboardHorizontalState)
        if (layoutSettings.dashboardVerticalState)
            centerSplit.restoreState(layoutSettings.dashboardVerticalState)
    }

    function resetLayout() {
        if (layoutSettings) {
            layoutSettings.dashboardHorizontalState = undefined
            layoutSettings.dashboardVerticalState = undefined
        }
        libraryPane.SplitView.preferredWidth = Math.max(280, dashboard.width * 0.25)
        biblePane.SplitView.preferredWidth = Math.max(300, dashboard.width * 0.27)
        previewPane.SplitView.preferredHeight = Math.max(350, dashboard.height * 0.53)
    }

    readonly property color background: "#171b1f"
    readonly property color panel: "#20252a"
    readonly property color panelHigh: "#282e33"
    readonly property color line: "#41484e"
    readonly property color textMain: "#f2f4f5"
    readonly property color textMuted: "#aab2b8"
    readonly property color accent: "#b9c7ff"

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
    property bool screenControlsExpanded: true
    property real mediaVolumeBeforeMute: 0.8
    property var screenBeingRenamed: null
    property var contextMediaItem: null

    function externalScreenCount() {
        let count = 0
        for (let index = 0; index < dashboard.controller.outputContext.screens.length; ++index) {
            if (!dashboard.controller.outputContext.screens[index].primary)
                ++count
        }
        return count
    }

    function ensureExternalOutputs() {
        if (dashboard.controller.outputContext.outputWindows.length === 0 && externalScreenCount() > 0)
            dashboard.controller.outputContext.enableAllScreens()
    }

    function setScreenMediaEnabled(screen, enabled) {
        if (enabled && !screen.selected) {
            const screenId = screen.id
            if (dashboard.controller.outputContext.toggleScreen(screenId, true)) {
                Qt.callLater(function() {
                    dashboard.controller.outputContext.setOutputMediaEnabled(screenId, true)
                })
            }
            return
        }
        if (screen.selected)
            dashboard.controller.outputContext.setOutputMediaEnabled(screen.id, enabled)
    }

    function openScreenRename(screen) {
        if (!screen.selected && !dashboard.controller.outputContext.toggleScreen(screen.id, true))
            return
        screenBeingRenamed = screen
        screenNameField.text = screen.name
        screenRenameDialog.open()
        screenNameField.forceActiveFocus()
        screenNameField.selectAll()
    }

    Component.onCompleted: Qt.callLater(function() {
        ensureExternalOutputs()
        restoreLayout()
    })

    Connections {
        target: dashboard.controller.outputContext
        function onScreensChanged() { Qt.callLater(dashboard.ensureExternalOutputs) }
    }

    Dialog {
        id: screenRenameDialog
        title: qsTr("Renomear monitor")
        modal: true
        anchors.centerIn: parent
        width: 430
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            if (dashboard.screenBeingRenamed && screenNameField.text.trim().length > 0)
                dashboard.controller.outputContext.setOutputDisplayName(
                            dashboard.screenBeingRenamed.id, screenNameField.text)
        }
        contentItem: ColumnLayout {
            spacing: 10
            Label {
                text: qsTr("Nome que será exibido no HolyScreen:")
                color: dashboard.textMain
            }
            TextField {
                id: screenNameField
                Layout.fillWidth: true
                placeholderText: qsTr("Ex.: Projetor principal")
                onAccepted: screenRenameDialog.accept()
            }
            Label {
                text: qsTr("O nome técnico do monitor no Windows não será alterado.")
                color: dashboard.textMuted
                font.pixelSize: 11
            }
        }
    }

    Menu {
        id: mediaContextMenu
        MenuItem {
            text: qsTr("Abrir local do arquivo")
            enabled: dashboard.contextMediaItem !== null
                     && (dashboard.contextMediaItem.path || "").length > 0
            onTriggered: dashboard.controller.mediaContext.openFileLocation(
                             dashboard.contextMediaItem.path)
        }
        MenuSeparator { }
        MenuItem {
            text: {
                const path = dashboard.contextMediaItem !== null
                           ? (dashboard.contextMediaItem.path || "") : ""
                const favorites = dashboard.controller.mediaContext.favoriteMedia
                for (let index = 0; index < favorites.length; ++index) {
                    if (favorites[index].path === path)
                        return qsTr("★ Remover dos favoritos")
                }
                return qsTr("☆ Adicionar aos favoritos")
            }
            enabled: dashboard.contextMediaItem !== null
                     && (dashboard.contextMediaItem.path || "").length > 0
            onTriggered: dashboard.controller.mediaContext.toggleFavoriteMedia(
                             dashboard.contextMediaItem.path)
        }
    }

    function openMediaContextMenu(item) {
        if (!item || !(item.path || "").length)
            return
        contextMediaItem = item
        mediaContextMenu.popup()
    }

    Rectangle { anchors.fill: parent; color: dashboard.background }

    SplitView {
        id: horizontalSplit
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

        LibraryPanel {
            id: libraryPane
            SplitView.preferredWidth: Math.max(280, dashboard.width * 0.25)
            SplitView.minimumWidth: 220
            controller: dashboard.controller.mediaContext
            backgroundColor: dashboard.background
            panelColor: dashboard.panel
            panelHighColor: dashboard.panelHigh
            lineColor: dashboard.line
            textMainColor: dashboard.textMain
            textMutedColor: dashboard.textMuted
            accentColor: dashboard.accent
            onOpenLibrary: dashboard.openLibrary()
            onShowMediaOptions: function(item) { dashboard.openMediaContextMenu(item) }
        }

        SplitView {
            id: centerSplit
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
                id: previewPane
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
                            outputLabel: qsTr("PRÉVIA")
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
                            isBlackout: dashboard.controller.outputContext.blackout
                            identifyVisible: dashboard.controller.outputContext.identifyVisible
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: dashboard.screenControlsExpanded ? 202 : 148
                        Layout.margins: 12; spacing: 6
                        Slider { Layout.fillWidth: true; from: 0; to: Math.max(1, dashboard.controller.mediaContext.mediaDurationMs); value: dashboard.controller.mediaContext.mediaPositionMs; onMoved: dashboard.controller.mediaContext.seekMedia(value) }
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: dashboard.duration(dashboard.controller.mediaContext.mediaPositionMs); color: dashboard.accent; font.pixelSize: 11 }
                            Item { Layout.fillWidth: true }
                            Label { text: dashboard.duration(dashboard.controller.mediaContext.mediaDurationMs); color: dashboard.textMuted; font.pixelSize: 11 }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            PlayerButton {
                                text: "⇄"
                                Accessible.name: qsTr("Embaralhar playlist")
                                ToolTip.visible: hovered
                                ToolTip.text: Accessible.name
                                onClicked: dashboard.controller.mediaContext.shuffleMediaPlaylist()
                            }
                            PlayerButton {
                                text: dashboard.controller.mediaContext.mediaRepeatMode === "one" ? "↻¹" : "↻"
                                highlighted: dashboard.controller.mediaContext.mediaRepeatMode !== "off"
                                Accessible.name: dashboard.controller.mediaContext.mediaRepeatMode === "off"
                                                 ? qsTr("Ativar repetição de toda a playlist")
                                                 : dashboard.controller.mediaContext.mediaRepeatMode === "all"
                                                   ? qsTr("Repetir somente o item atual")
                                                   : qsTr("Desativar repetição")
                                ToolTip.visible: hovered
                                ToolTip.text: Accessible.name
                                onClicked: dashboard.controller.mediaContext.mediaRepeatMode =
                                               dashboard.controller.mediaContext.mediaRepeatMode === "off" ? "all"
                                             : dashboard.controller.mediaContext.mediaRepeatMode === "all" ? "one" : "off"
                            }
                            Item { Layout.fillWidth: true }
                            PlayerButton {
                                text: "⏮"
                                Accessible.name: qsTr("Mídia anterior")
                                onClicked: dashboard.controller.mediaContext.previousMedia()
                            }
                            PlayerButton {
                                text: dashboard.controller.mediaContext.mediaState === "playing"
                                      || dashboard.controller.mediaContext.mediaState === "buffering" ? "⏸" : "▶"
                                highlighted: true
                                Accessible.name: dashboard.controller.mediaContext.mediaState === "playing"
                                                 || dashboard.controller.mediaContext.mediaState === "buffering"
                                                 ? qsTr("Pausar") : qsTr("Reproduzir")
                                onClicked: dashboard.controller.mediaContext.toggleMediaPause()
                            }
                            PlayerButton {
                                text: "■"
                                Accessible.name: qsTr("Parar")
                                onClicked: dashboard.controller.mediaContext.stopMedia()
                            }
                            PlayerButton {
                                text: "⏭"
                                Accessible.name: qsTr("Próxima mídia")
                                onClicked: dashboard.controller.mediaContext.nextMedia()
                            }
                            Item { Layout.fillWidth: true }
                            PlayerButton {
                                implicitWidth: 42
                                text: dashboard.controller.mediaContext.mediaVolume > 0.001 ? "🔊" : "🔇"
                                Accessible.name: dashboard.controller.mediaContext.mediaVolume > 0.001
                                                 ? qsTr("Mutar") : qsTr("Desmutar")
                                ToolTip.visible: hovered
                                ToolTip.text: Accessible.name
                                onClicked: {
                                    if (dashboard.controller.mediaContext.mediaVolume > 0.001) {
                                        dashboard.mediaVolumeBeforeMute = dashboard.controller.mediaContext.mediaVolume
                                        dashboard.controller.mediaContext.mediaVolume = 0
                                    } else {
                                        dashboard.controller.mediaContext.mediaVolume = Math.max(0.05,
                                                                          dashboard.mediaVolumeBeforeMute)
                                    }
                                }
                            }
                            Slider { Layout.preferredWidth: 110; from: 0; to: 1; value: dashboard.controller.mediaContext.mediaVolume; onMoved: dashboard.controller.mediaContext.mediaVolume = value }
                            PlayerButton {
                                implicitWidth: 42
                                text: dashboard.screenControlsExpanded ? "▾" : "▸"
                                Accessible.name: dashboard.screenControlsExpanded
                                                 ? qsTr("Ocultar seleção de telas")
                                                 : qsTr("Mostrar seleção de telas")
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
                                    text: qsTr("Exibir vídeo em:")
                                    color: dashboard.textMain
                                    font.bold: true
                                    font.pixelSize: 12
                                }
                                Repeater {
                                    model: dashboard.controller.outputContext.screens
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
                                        ToolTip.text: qsTr("Clique com o botão direito para renomear")
                                        onClicked: dashboard.setScreenMediaEnabled(screenCheckBox.modelData, checked)
                                        TapHandler {
                                            acceptedButtons: Qt.RightButton
                                            onTapped: dashboard.openScreenRename(screenCheckBox.modelData)
                                        }
                                    }
                                }
                                Label {
                                    visible: dashboard.externalScreenCount() === 0
                                    text: qsTr("nenhuma tela externa detectada")
                                    color: dashboard.textMuted
                                    font.pixelSize: 12
                                }
                                Button {
                                    visible: dashboard.externalScreenCount() > dashboard.controller.outputContext.outputWindows.length
                                    text: qsTr("Ativar todas")
                                    onClicked: dashboard.controller.outputContext.enableAllScreens()
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
                            Label { text: qsTr("▾  Reprodução"); color: dashboard.textMain; font.bold: true; font.pixelSize: 12 }
                            Item { Layout.fillWidth: true }
                            Button { text: qsTr("Salvar"); flat: true; onClicked: savePlaylistDialog.open() }
                            Button { text: qsTr("Limpar"); flat: true; onClicked: dashboard.controller.mediaContext.clearMediaPlaylist() }
                        }
                    }
                    ListView {
                        id: playlistList
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        spacing: 2
                        model: dashboard.controller.mediaContext.mediaPlaylist
                        delegate: Rectangle {
                            id: playlistDelegate
                            required property var modelData
                            required property int index
                            property real dragDistance: 0
                            width: ListView.view.width; height: 48
                            z: reorderDrag.active ? 10 : 0
                            radius: 4
                            color: reorderDrag.active ? "#40546b"
                                  : dashboard.controller.mediaContext.currentMediaId === playlistDelegate.modelData.id ? "#263b55"
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
                                    Accessible.name: qsTr("Reproduzir %1").arg(playlistDelegate.modelData.title)
                                    onClicked: dashboard.controller.mediaContext.playMedia(playlistDelegate.modelData.id)
                                }
                            }
                            HoverHandler { id: playlistTap }
                            TapHandler {
                                acceptedButtons: Qt.LeftButton
                                onDoubleTapped: dashboard.controller.mediaContext.playMedia(playlistDelegate.modelData.id)
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
                                        dashboard.controller.mediaContext.moveMedia(mediaId, targetIndex)
                                }
                            }
                        }
                    }
                }
            }
        }

        BiblePanel {
            id: biblePane
            SplitView.preferredWidth: Math.max(300, dashboard.width * 0.27)
            SplitView.minimumWidth: 280
            controller: dashboard.controller.bibleContext
            currentPresentationType: dashboard.controller.currentPresentationType
            currentSlideLabel: dashboard.controller.currentSlideLabel
            panelColor: dashboard.panel
            panelHighColor: dashboard.panelHigh
            lineColor: dashboard.line
            textMainColor: dashboard.textMain
            textMutedColor: dashboard.textMuted
            accentColor: dashboard.accent
            selectedColor: "#344d78"
            presentedColor: "#245d45"
            presentedBorderColor: "#58dc9a"
            onOpenBrowser: dashboard.openBibleBrowser()
            onOpenSettings: dashboard.openBible()
        }
    }

    FileDialog {
        id: savePlaylistDialog
        title: qsTr("Salvar playlist")
        fileMode: FileDialog.SaveFile
        nameFilters: ["Playlist M3U8 (*.m3u8)"]
        defaultSuffix: "m3u8"
        onAccepted: dashboard.controller.mediaContext.saveMediaPlaylist(selectedFile)
    }

    function duration(milliseconds) {
        const totalSeconds = Math.max(0, Math.floor((milliseconds || 0) / 1000))
        const minutes = Math.floor(totalSeconds / 60)
        const seconds = totalSeconds % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }
}
