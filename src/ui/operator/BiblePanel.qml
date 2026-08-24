pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: panel

    required property var controller
    required property string currentPresentationType
    required property string currentSlideLabel
    required property color panelColor
    required property color panelHighColor
    required property color lineColor
    required property color textMainColor
    required property color textMutedColor
    required property color accentColor
    required property color selectedColor
    required property color presentedColor
    required property color presentedBorderColor

    signal openBrowser()
    signal openSettings()

    property int selectedBookId: 1
    property int selectedChapter: 1
    property int selectedVerseIndex: -1

    readonly property var selectedTranslation: {
        for (let index = 0; index < panel.controller.bibleTranslations.length; ++index) {
            const translation = panel.controller.bibleTranslations[index]
            if (translation.id === panel.controller.biblePrimaryTranslationId)
                return translation
        }
        return panel.controller.bibleTranslations.length > 0
                ? panel.controller.bibleTranslations[0] : null
    }
    readonly property var chapterModel: {
        const translationId = panel.controller.biblePrimaryTranslationId
        return translationId && panel.selectedBookId > 0
                ? panel.controller.bibleChapterNumbers(panel.selectedBookId) : []
    }

    color: panel.panelColor
    border.color: panel.lineColor
    radius: 6

    function bibleBook(bookId) {
        for (let index = 0; index < panel.controller.bibleBooks.length; ++index) {
            const book = panel.controller.bibleBooks[index]
            if (book.id === bookId)
                return book
        }
        return null
    }

    function searchSelectedChapter() {
        const book = bibleBook(panel.selectedBookId)
        if (!book || panel.selectedChapter <= 0
                || !panel.controller.biblePrimaryTranslationId)
            return
        const verses = panel.controller.bibleVerseNumbers(panel.selectedBookId,
                                                          panel.selectedChapter)
        if (verses.length === 0)
            return
        panel.controller.bibleReferenceInput = book.name + " " + panel.selectedChapter
                + ":" + verses[0] + "-" + verses[verses.length - 1]
        panel.controller.searchBibleReference()
    }

    function selectBook(bookId) {
        panel.selectedBookId = Number(bookId)
        const chapters = panel.controller.bibleChapterNumbers(panel.selectedBookId)
        panel.selectedChapter = chapters.length > 0 ? Number(chapters[0]) : 0
        searchSelectedChapter()
    }

    function syncSelectors() {
        let reference = panel.controller.bibleReferenceInput
        if (panel.controller.bibleResults.length > 0)
            reference = panel.controller.bibleResults[0].label
        const match = /^(.+?)\s+(\d+)\s*(?::|\.)/.exec(reference.trim())
        if (!match)
            return
        const requestedBook = match[1].toLocaleLowerCase()
        for (let index = 0; index < panel.controller.bibleBooks.length; ++index) {
            const book = panel.controller.bibleBooks[index]
            if (book.name.toLocaleLowerCase() === requestedBook) {
                panel.selectedBookId = Number(book.id)
                panel.selectedChapter = Number(match[2])
                return
            }
        }
    }

    Component.onCompleted: Qt.callLater(function() {
        panel.syncSelectors()
        if (panel.controller.bibleResults.length === 0)
            panel.searchSelectedChapter()
    })

    Connections {
        target: panel.controller
        function onBibleResultsChanged() {
            panel.selectedVerseIndex = -1
            Qt.callLater(panel.syncSelectors)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: panel.panelHighColor
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                Label {
                    text: qsTr("▣  Bíblia Sagrada")
                    color: panel.textMainColor
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("NAVEGAR")
                    flat: true
                    onClicked: panel.openBrowser()
                }
                Button {
                    text: "☰"
                    flat: true
                    Accessible.name: qsTr("Configurar Bíblia")
                    onClicked: panel.openSettings()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 8
            ComboBox {
                id: bibleBookCombo
                Layout.fillWidth: true
                model: panel.controller.bibleBooks
                textRole: "name"
                valueRole: "id"
                currentIndex: {
                    for (let index = 0; index < panel.controller.bibleBooks.length; ++index) {
                        if (panel.controller.bibleBooks[index].id === panel.selectedBookId)
                            return index
                    }
                    return -1
                }
                onActivated: panel.selectBook(currentValue)
            }
            ComboBox {
                id: bibleChapterCombo
                Layout.preferredWidth: 76
                model: panel.chapterModel
                currentIndex: {
                    for (let index = 0; index < panel.chapterModel.length; ++index) {
                        if (Number(panel.chapterModel[index]) === panel.selectedChapter)
                            return index
                    }
                    return panel.chapterModel.length > 0 ? 0 : -1
                }
                onActivated: {
                    panel.selectedChapter = Number(currentValue)
                    panel.searchSelectedChapter()
                }
            }
            ComboBox {
                id: bibleTranslationCombo
                Layout.preferredWidth: 110
                model: panel.controller.bibleTranslations
                textRole: "abbreviation"
                valueRole: "id"
                currentIndex: {
                    for (let index = 0; index < panel.controller.bibleTranslations.length; ++index) {
                        if (panel.controller.bibleTranslations[index].id
                                === panel.controller.biblePrimaryTranslationId)
                            return index
                    }
                    return panel.controller.bibleTranslations.length > 0 ? 0 : -1
                }
                onActivated: {
                    panel.controller.biblePrimaryTranslationId = currentValue
                    panel.searchSelectedChapter()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: panel.lineColor
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 0
            model: panel.controller.bibleResults
            delegate: Rectangle {
                id: bibleVerseDelegate
                required property var modelData
                required property int index
                readonly property bool isSelected: panel.selectedVerseIndex === index
                readonly property bool isPresented:
                    panel.currentPresentationType === "bible"
                    && panel.currentSlideLabel === bibleVerseDelegate.modelData.label
                width: ListView.view.width
                height: verseText.implicitHeight + 30
                color: isPresented
                       ? panel.presentedColor
                       : isSelected ? panel.selectedColor
                       : index % 2 ? panel.panelHighColor : "transparent"
                border.color: isPresented
                              ? panel.presentedBorderColor
                              : isSelected ? panel.accentColor : "transparent"
                border.width: isPresented || isSelected ? 1 : 0
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10
                    Label {
                        text: bibleVerseDelegate.modelData.verse
                        color: panel.accentColor
                        font.bold: true
                        Layout.alignment: Qt.AlignTop
                    }
                    Label {
                        id: verseText
                        Layout.fillWidth: true
                        text: bibleVerseDelegate.modelData.versions
                              && bibleVerseDelegate.modelData.versions.length > 0
                              ? bibleVerseDelegate.modelData.versions[0].text
                              : bibleVerseDelegate.modelData.text
                        color: panel.textMainColor
                        wrapMode: Text.WordWrap
                        lineHeight: 1.25
                    }
                }
                TapHandler {
                    onTapped: panel.selectedVerseIndex = bibleVerseDelegate.index
                    onDoubleTapped: {
                        panel.selectedVerseIndex = bibleVerseDelegate.index
                        panel.controller.showBibleVerse(bibleVerseDelegate.index)
                    }
                }
            }
            Label {
                anchors.centerIn: parent
                visible: parent.count === 0
                text: panel.controller.bibleTranslations.length === 0
                      ? qsTr("Importe uma tradução bíblica para visualizar passagens")
                      : qsTr("Nenhum versículo encontrado")
                color: panel.textMutedColor
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: panel.panelHighColor
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 1
                Label {
                    text: panel.selectedTranslation
                          ? panel.selectedTranslation.displayName
                          : qsTr("Nenhuma tradução importada")
                    color: panel.textMainColor
                    font.bold: true
                    font.pixelSize: 10
                }
                Label {
                    text: panel.selectedTranslation
                          ? (panel.selectedTranslation.license === "public-domain"
                             ? qsTr("Domínio público")
                             : panel.selectedTranslation.license)
                          : qsTr("Use uma origem com licença adequada")
                    color: panel.textMutedColor
                    font.pixelSize: 9
                }
            }
        }
    }
}
