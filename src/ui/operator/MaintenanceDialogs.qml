pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

Item {
    id: dialogs

    required property var controller
    property url pendingRestoreSource

    function openRestore() {
        restoreDialog.open()
    }

    function openDiagnosticsExport() {
        diagnosticExportDialog.open()
    }

    FileDialog {
        id: restoreDialog
        title: qsTr("Selecionar backup do HolyScreen")
        nameFilters: [qsTr("Banco HolyScreen (*.db)")]
        onAccepted: {
            dialogs.pendingRestoreSource = selectedFile
            restoreConfirmDialog.open()
        }
    }

    FileDialog {
        id: diagnosticExportDialog
        title: qsTr("Exportar diagnóstico do HolyScreen")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Arquivo ZIP (*.zip)")]
        defaultSuffix: "zip"
        onAccepted: dialogs.controller.exportDiagnostics(selectedFile)
    }

    Dialog {
        id: restoreConfirmDialog
        title: qsTr("Agendar restauração?")
        modal: true
        width: UiScale.px(500)
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: dialogs.controller.scheduleRestore(
                        dialogs.pendingRestoreSource)
        Label {
            text: qsTr("O banco atual será preservado em um backup de segurança. A restauração será aplicada somente após reiniciar o app.")
            wrapMode: Text.WordWrap
        }
    }
}
