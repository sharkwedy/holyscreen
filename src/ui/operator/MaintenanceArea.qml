import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: area
    required property var context
    signal restoreRequested()
    signal diagnosticsExportRequested()
    spacing: 8

    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
    Label { text: qsTr("MANUTENÇÃO E DIAGNÓSTICOS"); color: "#8da0bc"; font.bold: true; font.pixelSize: UiScale.px(11) }
    Label {
        visible: area.context.recoveredFromCrash
        text: qsTr("Uma sessão anterior terminou inesperadamente. Um snapshot de recuperação foi criado.")
        color: "#ffba70"
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    RowLayout {
        Layout.fillWidth: true
        Button { text: qsTr("CRIAR BACKUP"); Layout.fillWidth: true; onClicked: area.context.createBackup() }
        Button { text: qsTr("RESTAURAR"); Layout.fillWidth: true; onClicked: area.restoreRequested() }
        Button { text: qsTr("EXPORTAR DIAGNÓSTICO"); Layout.fillWidth: true; onClicked: area.diagnosticsExportRequested() }
        Button {
            visible: area.context.debugEnabled && area.context.debugDiagnostics
            text: qsTr("BENCHMARK")
            Layout.fillWidth: true
            onClicked: area.context.runBenchmark()
        }
    }
    Label {
        visible: area.context.debugEnabled && area.context.debugDiagnostics
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        color: "#c8d5e8"
        text: qsTr("Versão %1 • Qt %2 • %3 • Telas %4 • Ops/s %5")
              .arg(area.context.diagnostics.version || "—")
              .arg(area.context.diagnostics.qtVersion || "—")
              .arg(area.context.diagnostics.platform || "—")
              .arg(area.context.diagnostics.detectedScreens || 0)
              .arg(area.context.diagnostics.benchmarkOperationsPerSecond || "—")
    }
    TextField {
        Layout.fillWidth: true
        placeholderText: qsTr("API oficial de Releases do GitHub")
        Accessible.name: placeholderText
        text: area.context.updateEndpoint
        readOnly: true
    }
    RowLayout {
        Layout.fillWidth: true
        Button { text: qsTr("VERIFICAR ATUALIZAÇÕES"); onClicked: area.context.checkForUpdates() }
        Button {
            text: area.context.updateDownloading
                ? qsTr("CANCELAR DOWNLOAD") : qsTr("BAIXAR ATUALIZAÇÃO")
            enabled: area.context.updateDownloading || area.context.updateDownloadable
            onClicked: area.context.updateDownloading
                ? area.context.cancelUpdateDownload() : area.context.downloadUpdate()
        }
        Button {
            text: qsTr("INSTALAR AGORA")
            visible: area.context.updateInstallable
            onClicked: area.context.installDownloadedUpdate()
        }
        Button {
            text: qsTr("MOSTRAR ARQUIVO")
            visible: area.context.updateDownloadedPath !== ""
            onClicked: area.context.revealUpdateDownload()
        }
    }
    ProgressBar {
        Layout.fillWidth: true
        visible: area.context.updateDownloading
        from: 0
        to: 1
        value: area.context.updateDownloadProgress
        Accessible.name: qsTr("Progresso do download da atualização")
    }
    Label {
        Layout.fillWidth: true
        text: area.context.updateStatus
        color: "#8da0bc"
        wrapMode: Text.WordWrap
    }
    Label {
        Layout.fillWidth: true
        visible: area.context.updateAvailable
        text: qsTr("Notas da versão: %1").arg(area.context.updateReleaseUrl)
        color: "#8da0bc"
        wrapMode: Text.WrapAnywhere
    }
    CheckBox {
        Layout.fillWidth: true
        text: qsTr("Verificar atualizações automaticamente")
        checked: area.context.automaticUpdateChecks
        onToggled: area.context.automaticUpdateChecks = checked
    }
    Label {
        Layout.fillWidth: true
        text: qsTr("Quando ligada, a verificação acontece na abertura e uma vez por dia. "
                 + "Nada é baixado nem instalado sem o seu clique.")
        color: "#6f7f92"
        font.pixelSize: UiScale.px(11)
        wrapMode: Text.WordWrap
    }
}
