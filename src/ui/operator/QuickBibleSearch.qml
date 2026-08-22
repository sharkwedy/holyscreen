import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: quickSearch
    required property var controller
    modal: true
    closePolicy: Popup.CloseOnEscape
    width: Math.min(parent ? parent.width - 80 : 760, 760)
    height: 390
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? (parent.height - height) / 2 : 0
    padding: 0

    function openWithText(initialText) {
        referenceInput.text = initialText
        errorLabel.text = ""
        open()
        Qt.callLater(function() {
            referenceInput.forceActiveFocus()
            referenceInput.cursorPosition = referenceInput.length
        })
    }

    function presentReference() {
        quickSearch.controller.bibleReferenceInput = referenceInput.text
        if (quickSearch.controller.searchBibleReference()) {
            quickSearch.controller.showBibleVerse(0)
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
        TextField {
            id: referenceInput
            Layout.fillWidth: true
            Layout.preferredHeight: 74
            horizontalAlignment: TextInput.AlignHCenter
            verticalAlignment: TextInput.AlignVCenter
            placeholderText: "Ex.: Lucas 1:1"
            placeholderTextColor: "#98a2aa"
            color: "#ffffff"
            font.pixelSize: 28
            font.bold: true
            selectByMouse: true
            background: Rectangle {
                color: "#20252a"
                border.color: referenceInput.activeFocus ? "#9fb3ff" : "#59636c"
                border.width: 2
                radius: 7
            }
            onAccepted: quickSearch.presentReference()
        }
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "Digite, por exemplo, João 3:16 e pressione Enter"
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
}
