pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: dashboard
    required property var controller
    property var layoutSettings: null
    signal openLibrary()
    signal openBible()
    signal openBibleBrowser()
    signal openThemeEditor(string scope)
    signal importAudio()
    signal importVideo()
    signal importImage()
    signal openOnlineLyrics(string key)

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
        rightPane.SplitView.preferredWidth = Math.max(300, dashboard.width * 0.27)
        previewPane.SplitView.preferredHeight = Math.max(350, dashboard.height * 0.53)
    }

    readonly property color background: "#171b1f"
    readonly property color panel: "#20252a"
    readonly property color panelHigh: "#282e33"
    readonly property color line: "#41484e"
    readonly property color textMain: "#f2f4f5"
    readonly property color textMuted: "#aab2b8"
    readonly property color accent: "#b9c7ff"

    property var contextMediaItem: null
    property bool biblePanelOverride: false

    Connections {
        target: dashboard.controller
        function onCurrentPresentationChanged() {
            if (dashboard.controller.currentPresentationType === "song")
                dashboard.biblePanelOverride = false
        }
    }

    Component.onCompleted: Qt.callLater(function() {
        restoreLayout()
    })

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
            onEditOnlineLyrics: function(key) { dashboard.openOnlineLyrics(key) }
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
            PlaybackPanel {
                id: previewPane
                SplitView.preferredHeight: Math.max(350, dashboard.height * 0.53)
                SplitView.minimumHeight: 280
                applicationController: dashboard.controller
                mediaController: dashboard.controller.mediaContext
                outputController: dashboard.controller.outputContext
                panelColor: dashboard.panel
                lineColor: dashboard.line
                textMainColor: dashboard.textMain
                textMutedColor: dashboard.textMuted
                accentColor: dashboard.accent
            }

            PlaylistPanel {
                SplitView.fillHeight: true
                SplitView.minimumHeight: 120
                controller: dashboard.controller.mediaContext
                panelColor: dashboard.panel
                panelHighColor: dashboard.panelHigh
                lineColor: dashboard.line
                textMainColor: dashboard.textMain
                textMutedColor: dashboard.textMuted
                accentColor: dashboard.accent
                onShowMediaOptions: function(item) { dashboard.openMediaContextMenu(item) }
            }
        }

        StackLayout {
            id: rightPane
            SplitView.preferredWidth: Math.max(300, dashboard.width * 0.27)
            SplitView.minimumWidth: 280
            currentIndex: dashboard.controller.currentPresentationType === "song"
                          && !dashboard.biblePanelOverride ? 1 : 0

            BiblePanel {
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
                onOpenThemes: dashboard.openThemeEditor("bible")
                onShowLyrics: dashboard.biblePanelOverride = false
            }

            LyricsPanel {
                controller: dashboard.controller
                panelColor: dashboard.panel
                panelHighColor: dashboard.panelHigh
                lineColor: dashboard.line
                textMainColor: dashboard.textMain
                textMutedColor: dashboard.textMuted
                accentColor: dashboard.accent
                selectedColor: "#344d78"
                presentedColor: "#245d45"
                presentedBorderColor: "#58dc9a"
                onShowBible: dashboard.biblePanelOverride = true
                onOpenThemes: dashboard.openThemeEditor("lyrics")
            }
        }
    }

}
