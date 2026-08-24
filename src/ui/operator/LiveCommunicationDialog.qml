pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog

    required property var controller
    required property real hostHeight

    title: qsTr("Comunicação ao vivo")
    modal: true
    width: 680
    height: Math.min(dialog.hostHeight - 80, 650)
    standardButtons: Dialog.Close

    contentItem: ScrollView {
        clip: true
        ColumnLayout {
            width: parent.width
            spacing: 14
            Label {
                text: qsTr("MENSAGEM NO TOPO")
                color: "#8da0bc"
                font.bold: true
            }
            TextField {
                id: audienceMessageEditor
                Layout.fillWidth: true
                placeholderText: qsTr("Mensagem para o público")
            }
            RowLayout {
                Button {
                    text: qsTr("EXIBIR")
                    onClicked: dialog.controller.setAudienceMessage(
                                   audienceMessageEditor.text)
                }
                Button {
                    text: qsTr("LIMPAR")
                    onClicked: dialog.controller.setAudienceMessage("")
                }
            }
            Label {
                text: qsTr("ALERTA CENTRAL")
                color: "#8da0bc"
                font.bold: true
            }
            TextField {
                id: alertEditor
                Layout.fillWidth: true
                placeholderText: qsTr("Aviso importante")
            }
            RowLayout {
                Button {
                    text: qsTr("EXIBIR ALERTA")
                    onClicked: dialog.controller.setAlertMessage(alertEditor.text)
                }
                Button {
                    text: qsTr("LIMPAR")
                    onClicked: dialog.controller.setAlertMessage("")
                }
            }
            Label {
                text: qsTr("LOWER THIRD")
                color: "#8da0bc"
                font.bold: true
            }
            TextField {
                id: lowerThirdTitleEditor
                Layout.fillWidth: true
                placeholderText: qsTr("Nome / título")
            }
            TextField {
                id: lowerThirdSubtitleEditor
                Layout.fillWidth: true
                placeholderText: qsTr("Descrição / igreja")
            }
            RowLayout {
                Button {
                    text: qsTr("EXIBIR LOWER THIRD")
                    onClicked: dialog.controller.setLowerThird(
                                   lowerThirdTitleEditor.text,
                                   lowerThirdSubtitleEditor.text)
                }
                Button {
                    text: qsTr("LIMPAR")
                    onClicked: dialog.controller.setLowerThird("", "")
                }
            }
            Label {
                text: qsTr("CONTAGEM REGRESSIVA")
                color: "#8da0bc"
                font.bold: true
            }
            RowLayout {
                Label { text: qsTr("Minutos") }
                SpinBox { id: countdownMinutes; from: 0; to: 999; value: 5 }
                Label { text: qsTr("Segundos") }
                SpinBox { id: countdownSeconds; from: 0; to: 59; value: 0 }
                Button {
                    text: dialog.controller.countdownRunning
                          ? dialog.controller.countdownText : qsTr("INICIAR")
                    onClicked: dialog.controller.startCountdown(
                                   countdownMinutes.value * 60
                                   + countdownSeconds.value)
                }
                Button {
                    text: qsTr("PARAR")
                    onClicked: dialog.controller.stopCountdown()
                }
            }
            Label {
                text: qsTr("CRONÔMETRO")
                color: "#8da0bc"
                font.bold: true
            }
            RowLayout {
                Label {
                    text: dialog.controller.stopwatchText
                    font.pixelSize: UiScale.px(22)
                    font.bold: true
                }
                Button {
                    text: dialog.controller.stopwatchRunning
                          ? qsTr("PAUSAR") : qsTr("INICIAR")
                    onClicked: dialog.controller.stopwatchRunning
                               ? dialog.controller.pauseStopwatch()
                               : dialog.controller.startStopwatch()
                }
                Button {
                    text: qsTr("ZERAR")
                    onClicked: dialog.controller.resetStopwatch()
                }
            }
        }
    }
}
