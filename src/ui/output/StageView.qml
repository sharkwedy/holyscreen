import QtQuick

Item {
    id: root

    property string currentText: ""
    property string nextText: ""
    property string clockText: ""
    property string message: ""
    property string countdownText: "00:00"
    property bool countdownRunning: false
    property string stopwatchText: "00:00"
    property bool stopwatchRunning: false

    Rectangle {
        anchors.fill: parent
        color: "#05070b"
    }

    Column {
        anchors.fill: parent
        anchors.margins: Math.max(32, Math.min(parent.width, parent.height) * 0.055)
        spacing: Math.max(16, parent.height * 0.025)

        Text {
            text: root.clockText + ((root.countdownRunning || root.stopwatchRunning)
                  ? "   •   " + (root.countdownRunning ? root.countdownText : root.stopwatchText) : "")
            color: "#5eead4"
            font.bold: true
            font.pixelSize: Math.max(34, root.height * 0.065)
        }

        Text {
            text: qsTr("ATUAL")
            color: "#94a3b8"
            font.bold: true
            font.letterSpacing: 3
            font.pixelSize: Math.max(18, root.height * 0.025)
        }

        Text {
            width: parent.width
            height: Math.max(120, root.height * 0.28)
            text: root.currentText.length > 0 ? root.currentText : "—"
            color: "#ffffff"
            wrapMode: Text.Wrap
            elide: Text.ElideRight
            font.bold: true
            font.pixelSize: Math.max(30, root.height * 0.058)
            verticalAlignment: Text.AlignVCenter
        }

        Rectangle { width: parent.width; height: 2; color: "#334155" }

        Text {
            text: qsTr("PRÓXIMO")
            color: "#94a3b8"
            font.bold: true
            font.letterSpacing: 3
            font.pixelSize: Math.max(18, root.height * 0.025)
        }

        Text {
            width: parent.width
            height: Math.max(90, root.height * 0.18)
            text: root.nextText.length > 0 ? root.nextText : qsTr("Fim da apresentação")
            color: "#cbd5e1"
            wrapMode: Text.Wrap
            elide: Text.ElideRight
            font.pixelSize: Math.max(24, root.height * 0.042)
            verticalAlignment: Text.AlignVCenter
        }
    }

    Rectangle {
        visible: root.message.length > 0
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Math.max(24, root.height * 0.04)
        height: Math.max(72, messageText.implicitHeight + 30)
        radius: 12
        color: "#f59e0b"

        Text {
            id: messageText
            anchors.fill: parent
            anchors.margins: 15
            text: root.message
            color: "#111827"
            font.bold: true
            font.pixelSize: Math.max(22, root.height * 0.035)
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
