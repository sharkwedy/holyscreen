pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: flow

    required property var controller
    required property real availableWidth
    required property real availableHeight
    readonly property var optionalBibleTranslations: [
        {"id": "", "displayName": qsTr("Nenhuma")}
    ].concat(flow.controller.bibleTranslations)

    function open() {
        bibleDialog.open()
    }

    function translationIndex(model, translationId) {
        for (let index = 0; index < model.length; ++index) {
            if (model[index].id === translationId)
                return index
        }
        return 0
    }
    FileDialog {
        id: bibleImportDialog
        title: qsTr("Importar JSON HolyScreen legado")
        nameFilters: [qsTr("HolyScreen Bíblia JSON (*.json)")]
        onAccepted: flow.controller.importBibleTranslation(selectedFile)
    }
    FolderDialog {
        id: bibleFolderDialog
        title: qsTr("Selecionar repositório, data/canonical ou pasta da tradução")
        onAccepted: flow.controller.importBibleFolder(selectedFolder)
    }
    Dialog {
        id: bibleOnlineImportDialog
        title: qsTr("Importar Bíblia de origem pública")
        modal: true
        width: 620
        standardButtons: Dialog.Close
        contentItem: ColumnLayout {
            spacing: 12
            Label {
                Layout.fillWidth: true
                text: qsTr("Use uma URL HTTPS pública. O repositório Git é clonado internamente; o ZIP é validado e extraído em staging temporário.")
                wrapMode: Text.WordWrap
                color: "#b8c6dc"
            }
            Label { text: qsTr("REPOSITÓRIO GIT HTTPS"); color: "#8da0bc"; font.bold: true }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: bibleGitUrl
                    Layout.fillWidth: true
                    placeholderText: "https://github.com/usuario/repositorio.git"
                }
                Button {
                    text: qsTr("IMPORTAR GIT")
                    enabled: !flow.controller.bibleImportRunning && bibleGitUrl.text.trim().length > 0
                    onClicked: {
                        if (flow.controller.importBibleGit(bibleGitUrl.text))
                            bibleOnlineImportDialog.close()
                    }
                }
            }
            Label { text: qsTr("ARQUIVO ZIP HTTPS"); color: "#8da0bc"; font.bold: true }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: bibleZipUrl
                    Layout.fillWidth: true
                    placeholderText: "https://exemplo.org/biblias.zip"
                }
                Button {
                    text: qsTr("IMPORTAR ZIP")
                    enabled: !flow.controller.bibleImportRunning && bibleZipUrl.text.trim().length > 0
                    onClicked: {
                        if (flow.controller.importBibleZip(bibleZipUrl.text))
                            bibleOnlineImportDialog.close()
                    }
                }
            }
        }
    }
    Dialog {
        id: bibleLicenseDialog
        title: qsTr("Confirmar licenças das traduções")
        modal: true
        width: 580
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: flow.controller.confirmBibleImportLicenses()
        contentItem: ColumnLayout {
            spacing: 10
            Label {
                Layout.fillWidth: true
                text: qsTr("As traduções abaixo não estão marcadas como domínio público. O HolyScreen não redistribui esse conteúdo. Confirme apenas se você tem permissão para importá-lo:")
                wrapMode: Text.WordWrap
                color: "#ffba70"
            }
            Label {
                Layout.fillWidth: true
                text: flow.controller.bibleImportLicenseWarning
                wrapMode: Text.WordWrap
                color: "#eff6ff"
                font.bold: true
            }
        }
    }
    Connections {
        target: flow.controller
        function onBibleImportStateChanged() {
            if (flow.controller.bibleImportRequiresLicenseConfirmation
                    && !flow.controller.bibleImportRunning
                    && !bibleLicenseDialog.visible)
                bibleLicenseDialog.open()
        }
    }
    Dialog {
        id: bibleDialog
        title: qsTr("Bíblia")
        modal: true
        width: Math.min(flow.availableWidth - 80, 940)
        height: Math.min(flow.availableHeight - 80, 680)
        x: (flow.availableWidth - width) / 2
        y: (flow.availableHeight - height) / 2
        standardButtons: Dialog.Close

        contentItem: ColumnLayout {
            spacing: 10
            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("TRADUÇÕES (ATÉ 3 SIMULTÂNEAS)"); color: "#8da0bc"; font.bold: true }
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("IMPORTAR PASTA")
                    enabled: !flow.controller.bibleImportRunning
                    onClicked: bibleFolderDialog.open()
                }
                Button {
                    text: qsTr("GIT / ZIP")
                    enabled: !flow.controller.bibleImportRunning
                    onClicked: bibleOnlineImportDialog.open()
                }
                Button {
                    text: qsTr("JSON LEGADO")
                    enabled: !flow.controller.bibleImportRunning
                    onClicked: bibleImportDialog.open()
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                visible: flow.controller.bibleImportRunning
                         || flow.controller.bibleImportMessage.length > 0
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        text: flow.controller.bibleImportMessage
                        color: flow.controller.bibleImportRunning ? "#70e1a7" : "#b8c6dc"
                        elide: Text.ElideRight
                    }
                    Button {
                        text: qsTr("CANCELAR")
                        visible: flow.controller.bibleImportRunning
                        onClicked: flow.controller.cancelBibleImport()
                    }
                }
                ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 100
                    value: flow.controller.bibleImportProgress
                    indeterminate: flow.controller.bibleImportRunning
                                   && flow.controller.bibleImportProgress === 0
                }
            }
            GridLayout {
                Layout.fillWidth: true
                columns: 3
                Label { text: qsTr("PRINCIPAL"); color: "#8da0bc" }
                Label { text: qsTr("SECUNDÁRIA"); color: "#8da0bc" }
                Label { text: qsTr("TERCEIRA"); color: "#8da0bc" }
                ComboBox {
                    Layout.fillWidth: true
                    model: flow.controller.bibleTranslations
                    textRole: "displayName"; valueRole: "id"
                    currentIndex: flow.translationIndex(model, flow.controller.biblePrimaryTranslationId)
                    onActivated: flow.controller.biblePrimaryTranslationId = currentValue
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: flow.optionalBibleTranslations
                    textRole: "displayName"; valueRole: "id"
                    currentIndex: flow.translationIndex(model, flow.controller.bibleSecondaryTranslationId)
                    onActivated: flow.controller.bibleSecondaryTranslationId = currentValue
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: flow.optionalBibleTranslations
                    textRole: "displayName"; valueRole: "id"
                    currentIndex: flow.translationIndex(model, flow.controller.bibleTertiaryTranslationId)
                    onActivated: flow.controller.bibleTertiaryTranslationId = currentValue
                }
            }
            RowLayout {
                Layout.fillWidth: true
                visible: flow.controller.bibleTranslations.length > 0
                Label { text: qsTr("ORIGEM:"); color: "#8da0bc"; font.bold: true }
                ComboBox {
                    id: bibleManagedTranslation
                    Layout.fillWidth: true
                    model: flow.controller.bibleTranslations
                    textRole: "displayName"
                    valueRole: "id"
                }
                Label {
                    text: bibleManagedTranslation.currentIndex >= 0
                          ? (bibleManagedTranslation.model[bibleManagedTranslation.currentIndex].license || qsTr("origem legada"))
                          : ""
                    color: "#8da0bc"
                }
                Button {
                    text: qsTr("ATUALIZAR DA ORIGEM")
                    enabled: !flow.controller.bibleImportRunning
                             && bibleManagedTranslation.currentIndex >= 0
                             && !!bibleManagedTranslation.model[bibleManagedTranslation.currentIndex].canUpdate
                    onClicked: flow.controller.updateBibleTranslationFromSource(
                                   bibleManagedTranslation.currentValue)
                }
            }
            Label {
                Layout.fillWidth: true
                visible: bibleManagedTranslation.currentIndex >= 0
                         && !!bibleManagedTranslation.model[bibleManagedTranslation.currentIndex].sourceLocation
                text: {
                    if (bibleManagedTranslation.currentIndex < 0)
                        return ""
                    const item = bibleManagedTranslation.model[bibleManagedTranslation.currentIndex]
                    const revision = item.sourceRevision
                                     ? qsTr(" • revisão %1").arg(item.sourceRevision.substring(0, 12)) : ""
                    const publisher = item.publisher ? " • " + item.publisher : ""
                    return qsTr("Origem: %1%2%3").arg(item.sourceLocation).arg(revision).arg(publisher)
                }
                color: "#64748b"
                elide: Text.ElideMiddle
                font.pixelSize: UiScale.px(11)
            }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    Layout.fillWidth: true
                    placeholderText: qsTr("João 3:16, Jo 3 16 ou João 3.16")
                    text: flow.controller.bibleReferenceInput
                    onTextEdited: flow.controller.bibleReferenceInput = text
                    onAccepted: flow.controller.searchBibleReference()
                }
                Button { text: qsTr("BUSCAR"); highlighted: true; onClicked: flow.controller.searchBibleReference() }
            }
            Label {
                visible: flow.controller.bibleTranslations.length === 0
                Layout.fillWidth: true
                text: qsTr("Importe uma pasta/repositório canônico, Git HTTPS, ZIP público ou JSON legado. Os textos bíblicos não são embutidos por questões de licenciamento.")
                color: "#ffba70"
                wrapMode: Text.WordWrap
            }
            ListView {
                id: bibleResultsList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 6
                model: flow.controller.bibleResults
                delegate: Rectangle {
                    id: bibleResultDelegate
                    required property var modelData
                    required property int index
                    width: ListView.view.width
                    height: Math.max(86, bibleResultText.implicitHeight + 24)
                    radius: 7
                    color: "#142137"
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        ColumnLayout {
                            Layout.fillWidth: true
                            Label { text: bibleResultDelegate.modelData.label; color: "#70e1a7"; font.bold: true }
                            Label {
                                id: bibleResultText
                                Layout.fillWidth: true
                                text: bibleResultDelegate.modelData.text
                                color: "#eff6ff"
                                wrapMode: Text.WordWrap
                            }
                        }
                        Button { text: qsTr("APRESENTAR"); onClicked: flow.controller.showBibleVerse(bibleResultDelegate.index) }
                    }
                }
                Label {
                    anchors.centerIn: parent
                    visible: bibleResultsList.count === 0 && flow.controller.bibleTranslations.length > 0
                    text: qsTr("Digite uma referência para localizar os versículos")
                    color: "#64748b"
                }
            }
        }
    }
}
