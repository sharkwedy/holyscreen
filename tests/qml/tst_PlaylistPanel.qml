import QtQuick
import QtTest
import ChurchPresenter

TestCase {
    id: testCase
    name: "PlaylistPanel"
    width: 900
    height: 320
    visible: true
    when: windowShown

    QtObject {
        id: fakeController
        property var mediaPlaylist: [{
            "id": "media-1",
            "title": "Vídeo de abertura",
            "durationMs": 92000
        }]
        property string currentMediaId: ""
        property string removedMediaId: ""

        function playMedia() {}
        function moveMedia() {}
        function clearMediaPlaylist() {}
        function saveMediaPlaylist() { return true }
        function removeMedia(id) {
            removedMediaId = id
            mediaPlaylist = []
        }
    }

    PlaylistPanel {
        id: panel
        parent: testCase
        width: 860
        height: 280
        controller: fakeController
        panelColor: "#171b1f"
        panelHighColor: "#22282d"
        lineColor: "#3a4249"
        textMainColor: "#f2f4f5"
        textMutedColor: "#a7b0b8"
        accentColor: "#8bbcff"
    }

    function init() {
        fakeController.removedMediaId = ""
        fakeController.mediaPlaylist = [{
            "id": "media-1",
            "title": "Vídeo de abertura",
            "durationMs": 92000
        }]
    }

    function test_removeButtonRemovesItsPlaylistItem() {
        const removeButton = findChild(panel, "removePlaylistItem-media-1")
        verify(removeButton !== null)
        compare(removeButton.text, "×")
        compare(removeButton.Accessible.name,
                "Remover Vídeo de abertura da playlist")

        mouseClick(removeButton)

        compare(fakeController.removedMediaId, "media-1")
        compare(fakeController.mediaPlaylist.length, 0)
    }
}
