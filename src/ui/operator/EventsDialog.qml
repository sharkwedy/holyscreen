pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Dialog {
    id: dialog

    required property var controller

    title: qsTr("Agenda do culto")
    modal: true
    width: Math.min(920, parent ? parent.width - 80 : 920)
    height: Math.min(720, parent ? parent.height - 80 : 720)
    anchors.centerIn: parent
    standardButtons: Dialog.Close

    background: Rectangle {
        color: "#20252a"
        border.color: "#46505a"
        radius: 8
    }

    contentItem: ScrollView {
        clip: true

        EventsArea {
            width: parent.width
            context: dialog.controller.eventContext
            sourceController: dialog.controller
            onClearHistoryRequested: clearHistoryDialog.open()
        }
    }

    Dialog {
        id: clearHistoryDialog
        anchors.centerIn: parent
        title: qsTr("Limpar histórico?")
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: dialog.controller.eventContext.clearHistory()

        contentItem: Label {
            text: qsTr("Essa ação remove definitivamente os registros de execução.")
            color: "#f2f4f5"
            wrapMode: Text.WordWrap
            width: 420
        }
    }
}
