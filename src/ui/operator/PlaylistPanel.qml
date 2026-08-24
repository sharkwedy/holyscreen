pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Rectangle {
    id: panel

    required property var controller
    required property color panelColor
    required property color panelHighColor
    required property color lineColor
    required property color textMainColor
    required property color textMutedColor
    required property color accentColor

    signal showMediaOptions(var item)

    color: panel.panelColor
    border.color: panel.lineColor
    radius: 6

    function duration(milliseconds) {
        const totalSeconds = Math.max(0, Math.floor((milliseconds || 0) / 1000))
        const minutes = Math.floor(totalSeconds / 60)
        const seconds = totalSeconds % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            color: panel.panelHighColor
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                Label {
                    text: qsTr("▾  Reprodução")
                    color: panel.textMainColor
                    font.bold: true
                    font.pixelSize: 12
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("Salvar")
                    flat: true
                    onClicked: savePlaylistDialog.open()
                }
                Button {
                    text: qsTr("Limpar")
                    flat: true
                    onClicked: panel.controller.clearMediaPlaylist()
                }
            }
        }

        ListView {
            id: playlistList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 2
            model: panel.controller.mediaPlaylist
            delegate: Rectangle {
                id: playlistDelegate
                required property var modelData
                required property int index
                property real dragDistance: 0
                width: ListView.view.width
                height: 48
                z: reorderDrag.active ? 10 : 0
                radius: 4
                color: reorderDrag.active ? "#40546b"
                      : panel.controller.currentMediaId === playlistDelegate.modelData.id
                          ? "#263b55"
                      : playlistHover.hovered ? "#30373d"
                      : (index % 2 ? "#1d2226" : "transparent")
                border.color: reorderDrag.active ? panel.accentColor : "transparent"
                transform: Translate { y: playlistDelegate.dragDistance }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    Label {
                        text: "☰"
                        color: panel.accentColor
                        font.pixelSize: 18
                        Layout.preferredWidth: 24
                    }
                    Label {
                        text: playlistDelegate.index + 1
                        color: panel.textMutedColor
                        Layout.preferredWidth: 24
                    }
                    Label {
                        Layout.fillWidth: true
                        text: playlistDelegate.modelData.title
                        color: panel.textMainColor
                        elide: Text.ElideRight
                    }
                    Label {
                        text: panel.duration(playlistDelegate.modelData.durationMs)
                        color: panel.textMutedColor
                        font.pixelSize: 11
                    }
                    PlayerButton {
                        implicitWidth: 38
                        implicitHeight: 36
                        font.pixelSize: 18
                        text: "▶"
                        Accessible.name: qsTr("Reproduzir %1").arg(
                                             playlistDelegate.modelData.title)
                        onClicked: panel.controller.playMedia(
                                       playlistDelegate.modelData.id)
                    }
                }
                HoverHandler { id: playlistHover }
                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    onDoubleTapped: panel.controller.playMedia(
                                        playlistDelegate.modelData.id)
                }
                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: panel.showMediaOptions(playlistDelegate.modelData)
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
                            panel.controller.moveMedia(mediaId, targetIndex)
                    }
                }
            }
        }
    }

    FileDialog {
        id: savePlaylistDialog
        title: qsTr("Salvar playlist")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Playlist M3U8 (*.m3u8)")]
        defaultSuffix: "m3u8"
        onAccepted: panel.controller.saveMediaPlaylist(selectedFile)
    }
}
