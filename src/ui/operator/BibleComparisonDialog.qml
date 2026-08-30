pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: comparison

    required property var controller
    property var comparisonModel: []

    title: qsTr("Comparar traduções bíblicas")
    modal: true
    width: Math.min(parent ? parent.width - 64 : 1100, 1100)
    height: Math.min(parent ? parent.height - 64 : 720, 720)
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? (parent.height - height) / 2 : 0
    standardButtons: Dialog.Close

    readonly property color panel: "#20252a"
    readonly property color panelHigh: "#292f35"
    readonly property color line: "#41484e"
    readonly property color textMain: "#f2f4f5"
    readonly property color textMuted: "#aab2b8"
    readonly property color accent: "#b9c7ff"

    function refreshComparison() {
        comparisonModel = controller.compareBibleReference(referenceField.text)
        comparisonError.text = comparisonModel.length === 0
                ? qsTr("Nenhum texto foi encontrado para essa referência") : ""
    }

    function openFor(reference) {
        referenceField.text = reference && reference.length > 0
                ? reference : controller.bibleReferenceInput
        if (referenceField.text.length > 0)
            refreshComparison()
        else
            comparisonModel = []
        open()
        referenceField.forceActiveFocus()
        referenceField.selectAll()
    }

    function presentVerse(label) {
        controller.bibleReferenceInput = label
        if (controller.searchBibleReference()) {
            controller.showBibleVerse(0)
            comparisonError.text = ""
        } else {
            comparisonError.text = controller.statusMessage
        }
    }

    background: Rectangle {
        color: comparison.panel
        border.color: comparison.line
        radius: 8
    }

    contentItem: ColumnLayout {
        spacing: 12

        Label {
            text: qsTr("Veja a mesma passagem em todas as traduções importadas")
            color: comparison.textMain
            font.bold: true
            font.pixelSize: UiScale.px(15)
        }

        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: referenceField
                Layout.fillWidth: true
                placeholderText: qsTr("Ex.: João 3:16")
                color: comparison.textMain
                placeholderTextColor: comparison.textMuted
                Accessible.name: qsTr("Referência para comparação")
                onAccepted: comparison.refreshComparison()
            }
            Button {
                text: qsTr("COMPARAR")
                highlighted: true
                onClicked: comparison.refreshComparison()
            }
        }

        Label {
            id: comparisonError
            Layout.fillWidth: true
            visible: text.length > 0
            color: "#ff9d9d"
            wrapMode: Text.WordWrap
        }

        ListView {
            id: translationsList
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: ListView.Horizontal
            spacing: 10
            clip: true
            model: comparison.comparisonModel
            ScrollBar.horizontal: ScrollBar { }

            delegate: Rectangle {
                id: translationCard
                required property var modelData
                width: Math.max(310, Math.min(400,
                           (translationsList.width - 20) /
                           Math.max(1, Math.min(3, translationsList.count))))
                height: translationsList.height - 14
                radius: 6
                color: comparison.panelHigh
                border.color: comparison.line

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    Label {
                        Layout.fillWidth: true
                        text: "%1 — %2".arg(translationCard.modelData.abbreviation)
                                           .arg(translationCard.modelData.name)
                        color: comparison.accent
                        font.bold: true
                        font.pixelSize: UiScale.px(14)
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        Layout.fillWidth: true
                        text: translationCard.modelData.language || ""
                        color: comparison.textMuted
                        font.pixelSize: UiScale.px(10)
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: comparison.line
                    }
                    ListView {
                        id: versesList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 8
                        clip: true
                        model: translationCard.modelData.verses
                        ScrollBar.vertical: ScrollBar { }

                        delegate: Rectangle {
                            id: verseCard
                            required property var modelData
                            width: versesList.width
                            height: verseContents.implicitHeight + 20
                            radius: 5
                            color: "#242b31"
                            border.color: verseHover.hovered
                                          ? comparison.accent : "transparent"

                            ColumnLayout {
                                id: verseContents
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 10
                                spacing: 6
                                RowLayout {
                                    Layout.fillWidth: true
                                    Label {
                                        Layout.fillWidth: true
                                        text: verseCard.modelData.label
                                        color: comparison.accent
                                        font.bold: true
                                    }
                                    Button {
                                        text: "▶"
                                        flat: true
                                        Accessible.name: qsTr("Apresentar %1").arg(
                                                             verseCard.modelData.label)
                                        ToolTip.visible: hovered
                                        ToolTip.text: Accessible.name
                                        onClicked: comparison.presentVerse(
                                                       verseCard.modelData.label)
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: verseCard.modelData.text
                                    color: comparison.textMain
                                    wrapMode: Text.WordWrap
                                    lineHeight: 1.25
                                }
                            }
                            HoverHandler { id: verseHover }
                        }
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: translationsList.count === 0
                         && comparisonError.text.length === 0
                text: qsTr("Digite uma referência para comparar")
                color: comparison.textMuted
            }
        }
    }
}
