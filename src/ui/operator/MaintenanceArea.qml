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
    Label { text: "MANUTENÇÃO E DIAGNÓSTICOS"; color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
    Label {
        visible: area.context.recoveredFromCrash
        text: "Uma sessão anterior terminou inesperadamente. Um snapshot de recuperação foi criado."
        color: "#ffba70"
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    RowLayout {
        Layout.fillWidth: true
        Button { text: "CRIAR BACKUP"; Layout.fillWidth: true; onClicked: area.context.createBackup() }
        Button { text: "RESTAURAR"; Layout.fillWidth: true; onClicked: area.restoreRequested() }
        Button { text: "EXPORTAR DIAGNÓSTICO"; Layout.fillWidth: true; onClicked: area.diagnosticsExportRequested() }
        Button {
            visible: area.context.debugEnabled && area.context.debugDiagnostics
            text: "BENCHMARK"
            Layout.fillWidth: true
            onClicked: area.context.runBenchmark()
        }
    }
    Label {
        visible: area.context.debugEnabled && area.context.debugDiagnostics
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        color: "#c8d5e8"
        text: "Versão " + (area.context.diagnostics.version || "—")
              + " • Qt " + (area.context.diagnostics.qtVersion || "—")
              + " • " + (area.context.diagnostics.platform || "—")
              + " • Telas " + (area.context.diagnostics.detectedScreens || 0)
              + " • Ops/s " + (area.context.diagnostics.benchmarkOperationsPerSecond || "—")
    }
    TextField {
        Layout.fillWidth: true
        placeholderText: "URL HTTPS do manifesto de atualização"
        Accessible.name: placeholderText
        text: area.context.updateEndpoint
        onEditingFinished: area.context.updateEndpoint = text
    }
    RowLayout {
        Layout.fillWidth: true
        Button { text: "VERIFICAR ATUALIZAÇÕES"; onClicked: area.context.checkForUpdates() }
        Label { Layout.fillWidth: true; text: area.context.updateStatus; color: "#8da0bc"; wrapMode: Text.WordWrap }
    }
}
