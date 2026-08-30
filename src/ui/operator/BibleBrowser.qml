pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: browser
    required property var controller
    property var selectedBook: null
    property int selectedChapter: 0
    property int selectedVerse: 0
    property var chapterModel: []
    property var verseModel: []

    title: qsTr("Navegação bíblica")
    modal: true
    width: parent ? parent.width : 1240
    height: parent ? parent.height : 760
    x: 0
    y: 0
    standardButtons: Dialog.Close

    readonly property color panel: "#20252a"
    readonly property color panelHigh: "#292f35"
    readonly property color line: "#41484e"
    readonly property color textMain: "#f2f4f5"
    readonly property color textMuted: "#aab2b8"
    readonly property color oldTestament: "#8a5727"
    readonly property color newTestament: "#187a56"
    readonly property color selectedColor: "#536fbd"

    function selectBook(book) {
        selectedBook = book
        selectedChapter = 0
        selectedVerse = 0
        chapterModel = controller.bibleChapterNumbers(book.id)
        verseModel = []
    }

    function selectChapter(chapter) {
        selectedChapter = chapter
        selectedVerse = 0
        verseModel = controller.bibleVerseNumbers(selectedBook.id, chapter)
    }

    function presentTypedReference() {
        controller.bibleReferenceInput = directReference.text
        if (controller.searchBibleReference()) {
            controller.showBibleVerse(0)
            browserError.text = ""
        } else {
            browserError.text = controller.statusMessage
        }
    }

    background: Rectangle {
        color: browser.panel
        border.color: browser.line
        radius: 0
    }

    contentItem: ColumnLayout {
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Digite uma referência ou escolha livro, capítulo e versículo")
                color: browser.textMain
                font.bold: true
                font.pixelSize: UiScale.px(15)
            }
            Item { Layout.fillWidth: true }
            Label {
                text: browser.controller.bibleTranslations.length > 0
                      ? qsTr("Tradução principal ativa") : qsTr("Nenhuma tradução importada")
                color: browser.controller.bibleTranslations.length > 0
                       ? "#8bdcb7" : "#ffba70"
                font.pixelSize: UiScale.px(11)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: directReference
                Layout.fillWidth: true
                placeholderText: qsTr("Ex.: João 3:16")
                color: browser.textMain
                placeholderTextColor: browser.textMuted
                onAccepted: browser.presentTypedReference()
            }
            Button {
                text: qsTr("APRESENTAR")
                highlighted: true
                onClicked: browser.presentTypedReference()
            }
        }
        Label {
            id: browserError
            Layout.fillWidth: true
            visible: text.length > 0
            color: "#ff9d9d"
            wrapMode: Text.WordWrap
        }

        Label { text: qsTr("LIVROS"); color: browser.textMuted; font.bold: true; font.pixelSize: UiScale.px(11) }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 260
            color: browser.panelHigh
            border.color: browser.line
            radius: 6
            GridView {
                id: booksGrid
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                cellWidth: Math.max(112, width / Math.max(1, Math.floor(width / 128)))
                cellHeight: 58
                model: browser.controller.bibleBooks
                ScrollBar.vertical: ScrollBar { }
                delegate: Rectangle {
                    id: bookDelegate
                    required property var modelData
                    width: booksGrid.cellWidth - 6
                    height: booksGrid.cellHeight - 6
                    radius: 5
                    color: browser.selectedBook !== null
                           && browser.selectedBook.id === modelData.id
                           ? browser.selectedColor
                           : modelData.testament === "old"
                             ? browser.oldTestament : browser.newTestament
                    border.color: bookHover.hovered ? "#f2f4f5" : "transparent"
                    Column {
                        anchors.centerIn: parent
                        width: parent.width - 12
                        spacing: 2
                        Label {
                            width: parent.width
                            text: bookDelegate.modelData.name
                            color: "white"
                            font.pixelSize: UiScale.px(14)
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                        }
                        Label {
                            width: parent.width
                            text: bookDelegate.modelData.testament === "old"
                                  ? qsTr("Antigo Testamento") : qsTr("Novo Testamento")
                            color: "#e5e7eb"
                            font.pixelSize: UiScale.px(9)
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                    HoverHandler { id: bookHover }
                    TapHandler { onTapped: browser.selectBook(bookDelegate.modelData) }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: browser.panelHigh
                border.color: browser.line
                radius: 6
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    Label {
                        text: browser.selectedBook === null
                              ? qsTr("CAPÍTULOS")
                              : qsTr("CAPÍTULOS — %1").arg(browser.selectedBook.name)
                        color: browser.textMain
                        font.bold: true
                    }
                    GridView {
                        id: chaptersGrid
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        cellWidth: 62
                        cellHeight: 50
                        model: browser.chapterModel
                        ScrollBar.vertical: ScrollBar { }
                        delegate: Button {
                            required property int modelData
                            width: chaptersGrid.cellWidth - 5
                            height: chaptersGrid.cellHeight - 5
                            text: modelData
                            highlighted: browser.selectedChapter === modelData
                            onClicked: browser.selectChapter(modelData)
                        }
                        Label {
                            anchors.centerIn: parent
                            visible: chaptersGrid.count === 0
                            text: browser.selectedBook === null
                                  ? qsTr("Selecione um livro")
                                  : qsTr("Nenhum capítulo disponível")
                            color: browser.textMuted
                        }
                    }
                }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: browser.panelHigh
                border.color: browser.line
                radius: 6
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    Label {
                        text: browser.selectedChapter > 0
                              ? qsTr("VERSÍCULOS — %1 %2").arg(browser.selectedBook.name)
                                .arg(browser.selectedChapter)
                              : qsTr("VERSÍCULOS")
                        color: browser.textMain
                        font.bold: true
                    }
                    GridView {
                        id: versesGrid
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        cellWidth: 62
                        cellHeight: 50
                        model: browser.verseModel
                        ScrollBar.vertical: ScrollBar { }
                        delegate: Button {
                            required property int modelData
                            width: versesGrid.cellWidth - 5
                            height: versesGrid.cellHeight - 5
                            text: modelData
                            highlighted: browser.selectedVerse === modelData
                            onClicked: {
                                browser.selectedVerse = modelData
                                if (!browser.controller.presentBibleReference(
                                        browser.selectedBook.id,
                                        browser.selectedChapter, modelData)) {
                                    browserError.text = browser.controller.statusMessage
                                } else {
                                    browserError.text = ""
                                }
                            }
                        }
                        Label {
                            anchors.centerIn: parent
                            visible: versesGrid.count === 0
                            text: browser.selectedChapter === 0
                                  ? qsTr("Selecione um capítulo")
                                  : qsTr("Nenhum versículo disponível")
                            color: browser.textMuted
                        }
                    }
                }
            }
        }
    }
}
