pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
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
    required property color selectedColor
    required property color presentedColor
    required property color presentedBorderColor

    signal showBible()
    signal openThemes()

    property int selectedSlideIndex: -1

    color: panel.panelColor
    border.color: panel.lineColor
    radius: 6

    Connections {
        target: panel.controller
        function onCurrentPresentationChanged() {
            panel.selectedSlideIndex = -1
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: panel.panelHighColor
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                Label {
                    Layout.fillWidth: true
                    text: qsTr("♫  %1").arg(
                              panel.controller.currentPresentationTitle
                              || qsTr("Letra"))
                    color: panel.textMainColor
                    font.bold: true
                    elide: Text.ElideRight
                }
                Button {
                    text: qsTr("TEMAS")
                    flat: true
                    onClicked: panel.openThemes()
                }
                Button {
                    text: qsTr("BÍBLIA")
                    flat: true
                    onClicked: panel.showBible()
                }
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.margins: 10
            text: qsTr("Clique duas vezes em uma parte para exibi-la nas telas")
            color: panel.textMutedColor
            font.pixelSize: UiScale.px(11)
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: panel.lineColor
        }

        ListView {
            id: lyricsSlides
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: panel.controller.textSlides
            spacing: 2

            delegate: Rectangle {
                id: lyricsSlide
                required property var modelData
                required property int index
                readonly property bool isSelected: panel.selectedSlideIndex === index
                readonly property bool isPresented:
                    panel.controller.currentPresentationType === "song"
                    && panel.controller.currentSlideId === lyricsSlide.modelData.id
                    && panel.controller.textVisible
                width: ListView.view.width
                height: Math.max(72, slideText.implicitHeight + 40)
                color: isPresented ? panel.presentedColor
                      : isSelected ? panel.selectedColor
                      : index % 2 ? panel.panelHighColor : "transparent"
                border.color: isPresented ? panel.presentedBorderColor
                              : isSelected ? panel.accentColor : "transparent"
                border.width: isPresented || isSelected ? 1 : 0

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 5
                    Label {
                        Layout.fillWidth: true
                        text: lyricsSlide.modelData.label
                              || qsTr("Parte %1").arg(lyricsSlide.index + 1)
                        color: panel.accentColor
                        font.bold: true
                        font.pixelSize: UiScale.px(11)
                    }
                    Label {
                        id: slideText
                        Layout.fillWidth: true
                        text: lyricsSlide.modelData.text || ""
                        color: panel.textMainColor
                        wrapMode: Text.WordWrap
                        lineHeight: 1.2
                    }
                }

                TapHandler {
                    onTapped: panel.selectedSlideIndex = lyricsSlide.index
                    onDoubleTapped: {
                        panel.selectedSlideIndex = lyricsSlide.index
                        panel.controller.showTextSlide(lyricsSlide.index)
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: lyricsSlides.count === 0
                text: qsTr("A letra selecionada não possui partes")
                color: panel.textMutedColor
            }
        }
    }
}
