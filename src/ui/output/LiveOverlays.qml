import QtQuick

Item {
    id: root
    property string message: ""
    property string alertMessage: ""
    property string lowerThirdTitle: ""
    property string lowerThirdSubtitle: ""
    property string countdownText: "00:00"
    property bool countdownRunning: false
    property string stopwatchText: "00:00"
    property bool stopwatchRunning: false

    Rectangle {
        visible: root.message.length > 0
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 36
        width: Math.min(parent.width * 0.82, messageLabel.implicitWidth + 80)
        height: messageLabel.implicitHeight + 30
        radius: height / 2
        color: "#dc2626"
        Text {
            id: messageLabel
            anchors.centerIn: parent
            width: parent.width - 48
            text: root.message
            color: "white"
            font.bold: true
            font.pixelSize: Math.max(22, root.height * 0.034)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
    }

    Rectangle {
        visible: root.alertMessage.length > 0
        anchors.centerIn: parent
        width: parent.width * 0.82
        height: Math.max(150, alertLabel.implicitHeight + 70)
        radius: 16
        color: "#eab308"
        border.color: "#fef08a"
        border.width: 3
        Text {
            id: alertLabel
            anchors.fill: parent
            anchors.margins: 30
            text: root.alertMessage
            color: "#111827"
            font.bold: true
            font.pixelSize: Math.max(34, root.height * 0.07)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
        }
    }

    Rectangle {
        visible: root.lowerThirdTitle.length > 0 || root.lowerThirdSubtitle.length > 0
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 52
        anchors.bottomMargin: 52
        width: Math.min(parent.width * 0.7, 900)
        height: 118
        radius: 10
        color: "#e60f172a"
        border.color: "#38bdf8"
        border.width: 2
        Column {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 5
            Text { text: root.lowerThirdTitle; color: "white"; font.bold: true; font.pixelSize: 34 }
            Text { text: root.lowerThirdSubtitle; color: "#bae6fd"; font.pixelSize: 23 }
        }
    }

    Rectangle {
        visible: root.countdownRunning || root.stopwatchRunning
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 36
        width: timerLabel.implicitWidth + 40
        height: timerLabel.implicitHeight + 22
        radius: 10
        color: "#d90f172a"
        Text {
            id: timerLabel
            anchors.centerIn: parent
            text: root.countdownRunning ? root.countdownText : root.stopwatchText
            color: "white"
            font.bold: true
            font.pixelSize: Math.max(28, root.height * 0.05)
        }
    }
}
