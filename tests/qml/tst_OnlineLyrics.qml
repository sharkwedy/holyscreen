import QtQuick
import QtTest
import ChurchPresenter

TestCase {
    id: testCase
    name: "OnlineLyrics"
    width: 1200
    height: 760
    visible: true
    when: windowShown

    QtObject {
        id: fakeController

        property var songs: [{"id": "local-1", "title": "Santo",
                              "author": "Comunidade", "slideCount": 3,
                              "sequenceCount": 3}]
        property string songSearch: "santo"
        property string audioFileSearch: ""
        property string videoFileSearch: ""
        property string imageFileSearch: ""
        property var folderAudioFiles: []
        property var folderVideoFiles: []
        property var folderImageFiles: []
        property var mediaFolders: []
        property var mediaPlaylist: []
        property string mediaState: "stopped"
        property var onlineLyricsResults: [{
            "key": "lrclib:42", "title": "Santo Pra Sempre",
            "artist": "Gabriel Guedes", "provider": "LRCLIB",
            "album": "", "lyrics": "[Refrão]\nSanto pra sempre",
            "hasLyrics": true, "savedCount": 0,
            "sourceUrl": "https://lrclib.net/api/get/42"
        }]
        property bool onlineLyricsBusy: false
        property string onlineLyricsError: ""
        property string onlineLyricsStatus: ""
        property bool vagalumeApiKeyConfigured: false
        property string lyricsSecretStorageName: "Windows Credential Manager"
        property string savedKey: ""
        property string editedTitle: ""
        property string onlineSearchQuery: ""

        signal onlineLyricsLoaded(string key)

        function cancelOnlineLyricsSearch() {}
        function searchOnlineLyrics(query) { onlineSearchQuery = query }
        function selectSong() {}
        function addCatalogFileToPlaylist() { return "" }
        function saveOnlineLyrics(key) { savedKey = key; return "song-2" }
        function onlineLyricsResult(key) {
            for (let index = 0; index < onlineLyricsResults.length; ++index) {
                if (onlineLyricsResults[index].key === key)
                    return onlineLyricsResults[index]
            }
            return ({})
        }
        function loadOnlineLyrics() {}
        function setVagalumeApiKey() { return true }
        function clearVagalumeApiKey() { return true }
        function openOnlineLyricsSource() { return true }
        function saveEditedOnlineLyrics(key, title, artist, lyrics) {
            savedKey = key
            editedTitle = title
            return lyrics.length > 0 ? "song-3" : ""
        }
    }

    LibraryPanel {
        id: library
        parent: testCase
        width: 520
        height: 700
        selectedTab: 0
        searchText: "santo"
        controller: fakeController
        backgroundColor: "#111820"
        panelColor: "#171b1f"
        panelHighColor: "#22282d"
        lineColor: "#3a4249"
        textMainColor: "#f2f4f5"
        textMutedColor: "#a7b0b8"
        accentColor: "#8bbcff"
    }

    OnlineLyricsDialog {
        id: editorDialog
        parent: testCase
        controller: fakeController
        hostWidth: testCase.width
        hostHeight: testCase.height
    }

    function init() {
        fakeController.savedKey = ""
        fakeController.editedTitle = ""
        fakeController.onlineSearchQuery = ""
        fakeController.songSearch = "santo"
        library.selectedTab = 0
        library.searchText = "santo"
        library.searchDraft = "santo"
    }

    function test_localResultsComeBeforeOnlineResultsAndQuickSave() {
        const items = library.lyricsModel()
        compare(items.length, 3)
        compare(items[0].kind, "local")
        compare(items[0].id, "local-1")
        compare(items[1].kind, "onlineHeader")
        compare(items[2].kind, "online")
        compare(items[2].key, "lrclib:42")

        library.activateItem(items[2])
        compare(fakeController.savedKey, "lrclib:42")
    }

    function test_editorLoadsDraftAndSavesEditedLyrics() {
        editorDialog.openFor("lrclib:42", "santo")
        tryCompare(editorDialog, "opened", true)

        const title = findChild(editorDialog, "onlineLyricsTitleField")
        const lyrics = findChild(editorDialog, "onlineLyricsEditor")
        const saveButton = findChild(editorDialog, "saveOnlineLyricsButton")
        verify(title !== null)
        verify(lyrics !== null)
        verify(saveButton !== null)
        compare(title.text, "Santo Pra Sempre")
        verify(lyrics.text.indexOf("Santo pra sempre") >= 0)

        title.text = "Santo Pra Sempre (editada)"
        mouseClick(saveButton)
        compare(fakeController.savedKey, "lrclib:42")
        compare(fakeController.editedTitle, "Santo Pra Sempre (editada)")
        editorDialog.close()
    }

    function test_searchIsAppliedOnlyAfterConfirmation() {
        const field = findChild(library, "librarySearchField")
        const confirm = findChild(library, "confirmLibrarySearchButton")
        verify(field !== null)
        verify(confirm !== null)

        field.forceActiveFocus()
        field.selectAll()
        keyClicks(field, "graca")
        compare(fakeController.songSearch, "santo")

        mouseClick(confirm)
        compare(fakeController.songSearch, "graca")
        compare(library.searchText, "graca")
    }
}
