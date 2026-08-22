import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: quickSearch
    required property var controller
    property int selectedBookId: -1
    modal: true
    closePolicy: Popup.CloseOnEscape
    width: Math.min(parent ? parent.width - 80 : 760, 760)
    height: 390
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? (parent.height - height) / 2 : 0
    padding: 0

    function normalizedBookName(value) {
        return String(value || "").normalize("NFD")
                .replace(/[\u0300-\u036f]/g, "").toLocaleLowerCase().trim()
    }

    function matchingBooks(value) {
        const prefix = normalizedBookName(value)
        const matches = []
        if (prefix.length === 0)
            return matches
        for (let index = 0; index < controller.bibleBooks.length; ++index) {
            const book = controller.bibleBooks[index]
            if (normalizedBookName(book.name).startsWith(prefix))
                matches.push(book)
        }
        return matches
    }

    function completeBook(moveToChapter) {
        const matches = matchingBooks(bookInput.text)
        if (matches.length !== 1) {
            selectedBookId = -1
            return false
        }
        const book = matches[0]
        selectedBookId = Number(book.id)
        bookInput.text = book.name
        bookInput.cursorPosition = bookInput.length
        errorLabel.text = ""
        if (moveToChapter)
            Qt.callLater(function() { chapterInput.forceActiveFocus() })
        return true
    }

    function chapterAvailable(chapter) {
        if (selectedBookId < 1 || chapter < 1)
            return false
        const chapters = controller.bibleChapterNumbers(selectedBookId)
        for (let index = 0; index < chapters.length; ++index) {
            if (Number(chapters[index]) === chapter)
                return true
        }
        return false
    }

    function acceptChapter() {
        if (selectedBookId < 1 && !completeBook(false)) {
            errorLabel.text = "Selecione um livro antes do capítulo."
            bookInput.forceActiveFocus()
            return false
        }
        const chapter = Number(chapterInput.text)
        if (!chapterAvailable(chapter)) {
            errorLabel.text = "Capítulo indisponível para o livro selecionado."
            chapterInput.forceActiveFocus()
            chapterInput.selectAll()
            return false
        }
        errorLabel.text = ""
        Qt.callLater(function() { verseInput.forceActiveFocus() })
        return true
    }

    function openWithText(initialText) {
        selectedBookId = -1
        bookInput.text = initialText || ""
        chapterInput.text = ""
        verseInput.text = ""
        errorLabel.text = ""
        open()
        Qt.callLater(function() {
            if (!completeBook(true)) {
                bookInput.forceActiveFocus()
                bookInput.cursorPosition = bookInput.length
            }
        })
    }

    function presentReference() {
        if (selectedBookId < 1 && !completeBook(false)) {
            errorLabel.text = "Digite um nome de livro válido."
            bookInput.forceActiveFocus()
            return
        }
        const chapter = Number(chapterInput.text)
        const verse = Number(verseInput.text)
        if (!chapterAvailable(chapter)) {
            errorLabel.text = "Digite um capítulo válido."
            chapterInput.forceActiveFocus()
            return
        }
        const verses = quickSearch.controller.bibleVerseNumbers(selectedBookId, chapter)
        let verseIndex = -1
        for (let index = 0; index < verses.length; ++index) {
            if (Number(verses[index]) === verse) {
                verseIndex = index
                break
            }
        }
        if (verseIndex < 0) {
            errorLabel.text = "Digite um versículo válido."
            verseInput.forceActiveFocus()
            verseInput.selectAll()
            return
        }
        quickSearch.controller.bibleReferenceInput = bookInput.text + " " + chapter
                + ":" + verses[0] + "-" + verses[verses.length - 1]
        if (quickSearch.controller.searchBibleReference()) {
            quickSearch.controller.showBibleVerse(verseIndex)
            close()
        } else {
            errorLabel.text = quickSearch.controller.statusMessage
        }
    }

    background: Rectangle {
        color: "#30363b"
        border.color: "#65717b"
        border.width: 1
        radius: 10
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 12
        Label {
            Layout.alignment: Qt.AlignRight
            text: "Esc para cancelar"
            color: "#c5cbd0"
            font.pixelSize: 12
        }
        Item { Layout.preferredHeight: 4 }
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "BUSCA RÁPIDA DA BÍBLIA"
            color: "#b9c7ff"
            font.bold: true
            font.pixelSize: 13
        }
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "Livro, capítulo e versículo"
            color: "#f2f4f5"
            font.pixelSize: 30
            font.bold: true
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5
                Label { text: "Livro"; color: "#c5cbd0"; font.bold: true }
                TextField {
                    id: bookInput
                    Layout.fillWidth: true
                    Layout.preferredHeight: 70
                    Accessible.name: "Livro"
                    placeholderText: "Ex.: luc"
                    placeholderTextColor: "#98a2aa"
                    color: "#ffffff"
                    font.pixelSize: 25
                    font.bold: true
                    selectByMouse: true
                    background: Rectangle {
                        color: "#20252a"
                        border.color: bookInput.activeFocus ? "#9fb3ff" : "#59636c"
                        border.width: 2
                        radius: 7
                    }
                    onTextEdited: {
                        quickSearch.selectedBookId = -1
                        chapterInput.text = ""
                        verseInput.text = ""
                        errorLabel.text = ""
                        bookAdvance.restart()
                    }
                    onAccepted: {
                        bookAdvance.stop()
                        if (!quickSearch.completeBook(true))
                            errorLabel.text = "Continue digitando até identificar um único livro."
                    }
                }
            }
            ColumnLayout {
                Layout.preferredWidth: 150
                spacing: 5
                Label { text: "Capítulo"; color: "#c5cbd0"; font.bold: true }
                TextField {
                    id: chapterInput
                    Layout.fillWidth: true
                    Layout.preferredHeight: 70
                    Accessible.name: "Capítulo"
                    horizontalAlignment: TextInput.AlignHCenter
                    placeholderText: "1"
                    placeholderTextColor: "#98a2aa"
                    color: "#ffffff"
                    font.pixelSize: 25
                    font.bold: true
                    selectByMouse: true
                    validator: IntValidator { bottom: 1; top: 999 }
                    background: Rectangle {
                        color: "#20252a"
                        border.color: chapterInput.activeFocus ? "#9fb3ff" : "#59636c"
                        border.width: 2
                        radius: 7
                    }
                    onTextEdited: {
                        verseInput.text = ""
                        errorLabel.text = ""
                        chapterAdvance.restart()
                    }
                    onAccepted: {
                        chapterAdvance.stop()
                        quickSearch.acceptChapter()
                    }
                }
            }
            ColumnLayout {
                Layout.preferredWidth: 150
                spacing: 5
                Label { text: "Versículo"; color: "#c5cbd0"; font.bold: true }
                TextField {
                    id: verseInput
                    Layout.fillWidth: true
                    Layout.preferredHeight: 70
                    Accessible.name: "Versículo"
                    horizontalAlignment: TextInput.AlignHCenter
                    placeholderText: "1"
                    placeholderTextColor: "#98a2aa"
                    color: "#ffffff"
                    font.pixelSize: 25
                    font.bold: true
                    selectByMouse: true
                    validator: IntValidator { bottom: 1; top: 999 }
                    background: Rectangle {
                        color: "#20252a"
                        border.color: verseInput.activeFocus ? "#9fb3ff" : "#59636c"
                        border.width: 2
                        radius: 7
                    }
                    onTextEdited: errorLabel.text = ""
                    onAccepted: quickSearch.presentReference()
                }
            }
        }
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "Digite o início do livro; ao ficar único, o foco avança automaticamente"
            color: "#c5cbd0"
            font.pixelSize: 13
        }
        Label {
            id: errorLabel
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            visible: text.length > 0
            color: "#ff9d9d"
            wrapMode: Text.WordWrap
        }
        Item { Layout.fillHeight: true }
    }

    Timer {
        id: bookAdvance
        interval: 250
        repeat: false
        onTriggered: {
            if (bookInput.text.length > 0)
                quickSearch.completeBook(true)
        }
    }

    Timer {
        id: chapterAdvance
        interval: 450
        repeat: false
        onTriggered: {
            if (chapterInput.text.length > 0)
                quickSearch.acceptChapter()
        }
    }
}
