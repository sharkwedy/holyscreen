import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window

pragma ComponentBehavior: Bound

ApplicationWindow {
    id: root

    // O ApplicationController chega como propriedade de contexto criada pelo
    // main.cpp; o qmllint não conhece esse tipo, então a exceção fica restrita
    // a esta linha.
    // qmllint disable unqualified
    readonly property var controller: presentationController
    // qmllint enable unqualified
    visible: true
    onClosing: {
        operatorWindowSettings.savedWidth = width
        operatorWindowSettings.savedHeight = height
        operatorDashboard.saveLayout()
        Qt.quit()
    }
    color: "#0b1220"
    title: qsTr("HolyScreen — Operação")
    width: 1360
    height: 820
    minimumWidth: 1100
    minimumHeight: 680
    property url pendingRestoreSource
    readonly property var optionalBibleTranslations: [{"id":"", "displayName":qsTr("Nenhuma")}]
                                                     .concat(root.controller.bibleContext.bibleTranslations)

    Settings {
        id: operatorWindowSettings
        category: "OperatorWindow"
        property int savedWidth: 1360
        property int savedHeight: 820
        property var dashboardHorizontalState
        property var dashboardVerticalState
    }

    function translationIndex(model, translationId) {
        for (let index = 0; index < model.length; ++index) {
            if (model[index].id === translationId)
                return index
        }
        return 0
    }

    Instantiator {
        model: root.controller.outputContext.outputWindows
        delegate: OutputWindow {
            required property var modelData
            targetScreenIndex: modelData.screenIndex
            targetScreenX: modelData.screenX
            targetScreenY: modelData.screenY
            targetScreenWidth: modelData.screenWidth
            targetScreenHeight: modelData.screenHeight
            outputDisplayName: modelData.displayName
            identifier: modelData.identifier
            bibleTranslationId: modelData.bibleTranslationId
            outputRole: modelData.role
            mediaEnabled: modelData.mediaEnabled
            broadcastProfile: modelData.broadcast
        }
    }

    Dialog {
        id: stageMessageDialog
        title: qsTr("Comunicação com o palco")
        modal: true
        width: 560
        standardButtons: Dialog.Close
        onOpened: stageMessageEditor.text = root.controller.stageMessage
        contentItem: ColumnLayout {
            spacing: 12
            Label {
                text: qsTr("A mensagem aparece somente nas telas configuradas como palco.")
                color: "#8da0bc"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            TextArea {
                id: stageMessageEditor
                Layout.fillWidth: true
                Layout.preferredHeight: 110
                placeholderText: qsTr("Ex.: Pastor, faltam 5 minutos")
                wrapMode: TextEdit.Wrap
            }
            RowLayout {
                Layout.fillWidth: true
                Button {
                    text: qsTr("ENVIAR AO PALCO")
                    enabled: stageMessageEditor.text.trim().length > 0
                    onClicked: root.controller.stageMessage = stageMessageEditor.text
                }
                Button {
                    text: qsTr("LIMPAR")
                    enabled: root.controller.stageMessage.length > 0
                    onClicked: {
                        stageMessageEditor.text = ""
                        root.controller.stageMessage = ""
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }
    }

    IntegrationsArea {
        id: integrationsArea
        controller: root.controller.integrationContext
    }

    AutomationsArea {
        id: automationsArea
        controller: root.controller.automationContext
    }

    EventsDialog {
        id: eventsDialog
        controller: root.controller
    }

    SettingsDialog {
        id: settingsDialog
        controller: root.controller
        onOpenLibrary: mediaLibraryDialog.open()
        onChooseBackground: wallpaperDialog.open()
        onRestoreLayout: operatorDashboard.resetLayout()
    }
    OnboardingDialog {
        id: onboardingDialog
        controller: root.controller
        onOpenSettings: function(tabIndex) { settingsDialog.openTab(tabIndex) }
        onOpenBible: bibleBrowser.open()
    }
    Dialog {
        id: liveDialog
        title: qsTr("Comunicação ao vivo")
        modal: true
        width: 680
        height: Math.min(root.height - 80, 650)
        standardButtons: Dialog.Close
        contentItem: ScrollView {
            clip: true
            ColumnLayout {
                width: parent.width
                spacing: 14
                Label { text: qsTr("MENSAGEM NO TOPO"); color: "#8da0bc"; font.bold: true }
                TextField { id: audienceMessageEditor; Layout.fillWidth: true; placeholderText: qsTr("Mensagem para o público") }
                RowLayout {
                    Button { text: qsTr("EXIBIR"); onClicked: root.controller.setAudienceMessage(audienceMessageEditor.text) }
                    Button { text: qsTr("LIMPAR"); onClicked: root.controller.setAudienceMessage("") }
                }
                Label { text: qsTr("ALERTA CENTRAL"); color: "#8da0bc"; font.bold: true }
                TextField { id: alertEditor; Layout.fillWidth: true; placeholderText: qsTr("Aviso importante") }
                RowLayout {
                    Button { text: qsTr("EXIBIR ALERTA"); onClicked: root.controller.setAlertMessage(alertEditor.text) }
                    Button { text: qsTr("LIMPAR"); onClicked: root.controller.setAlertMessage("") }
                }
                Label { text: qsTr("LOWER THIRD"); color: "#8da0bc"; font.bold: true }
                TextField { id: lowerThirdTitleEditor; Layout.fillWidth: true; placeholderText: qsTr("Nome / título") }
                TextField { id: lowerThirdSubtitleEditor; Layout.fillWidth: true; placeholderText: qsTr("Descrição / igreja") }
                RowLayout {
                    Button { text: qsTr("EXIBIR LOWER THIRD"); onClicked: root.controller.setLowerThird(lowerThirdTitleEditor.text, lowerThirdSubtitleEditor.text) }
                    Button { text: qsTr("LIMPAR"); onClicked: root.controller.setLowerThird("", "") }
                }
                Label { text: qsTr("CONTAGEM REGRESSIVA"); color: "#8da0bc"; font.bold: true }
                RowLayout {
                    Label { text: qsTr("Minutos") }
                    SpinBox { id: countdownMinutes; from: 0; to: 999; value: 5 }
                    Label { text: qsTr("Segundos") }
                    SpinBox { id: countdownSeconds; from: 0; to: 59; value: 0 }
                    Button {
                        text: root.controller.countdownRunning ? root.controller.countdownText : qsTr("INICIAR")
                        onClicked: root.controller.startCountdown(countdownMinutes.value * 60 + countdownSeconds.value)
                    }
                    Button { text: qsTr("PARAR"); onClicked: root.controller.stopCountdown() }
                }
                Label { text: qsTr("CRONÔMETRO"); color: "#8da0bc"; font.bold: true }
                RowLayout {
                    Label { text: root.controller.stopwatchText; font.pixelSize: 22; font.bold: true }
                    Button {
                        text: root.controller.stopwatchRunning ? qsTr("PAUSAR") : qsTr("INICIAR")
                        onClicked: root.controller.stopwatchRunning
                                   ? root.controller.pauseStopwatch()
                                   : root.controller.startStopwatch()
                    }
                    Button { text: qsTr("ZERAR"); onClicked: root.controller.resetStopwatch() }
                }
            }
        }
    }

    FileDialog {
        id: wallpaperDialog
        title: qsTr("Selecionar wallpaper")
        nameFilters: [qsTr("Imagens (*.jpg *.jpeg *.png *.webp)")]
        onAccepted: root.controller.wallpaperSource = selectedFile
    }
    Dialog {
        id: clearHistoryDialog
        title: qsTr("Limpar histórico?")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: root.controller.eventContext.clearHistory()
        Label { text:qsTr("Essa ação remove definitivamente os registros de execução.");wrapMode:Text.WordWrap }
    }
    FileDialog {
        id: restoreDialog
        title:qsTr("Selecionar backup do HolyScreen")
        nameFilters:[qsTr("Banco HolyScreen (*.db)")]
        onAccepted:{root.pendingRestoreSource=selectedFile;restoreConfirmDialog.open()}
    }
    FileDialog {
        id: diagnosticExportDialog
        title: qsTr("Exportar diagnóstico do HolyScreen")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Arquivo ZIP (*.zip)")]
        defaultSuffix: "zip"
        onAccepted: root.controller.maintenanceContext.exportDiagnostics(selectedFile)
    }
    Dialog {
        id:restoreConfirmDialog;title:qsTr("Agendar restauração?");modal:true
        standardButtons:Dialog.Ok|Dialog.Cancel
        onAccepted:root.controller.maintenanceContext.scheduleRestore(root.pendingRestoreSource)
        Label {text:qsTr("O banco atual será preservado em um backup de segurança. A restauração será aplicada somente após reiniciar o app.");wrapMode:Text.WordWrap;width:420}
    }

    FileDialog {
        id: audioDialog
        title: qsTr("Importar áudios")
        fileMode: FileDialog.OpenFiles
        nameFilters: [qsTr("Áudios (*.mp3 *.wav *.flac *.m4a *.aac *.ogg *.opus *.wma *.aiff *.aif)")]
        onAccepted: root.controller.importAudioFiles(selectedFiles)
    }

    FileDialog {
        id: videoDialog
        title: qsTr("Importar vídeos")
        fileMode: FileDialog.OpenFiles
        nameFilters: [qsTr("Vídeos (*.mp4 *.mov *.m4v *.mkv *.webm *.avi *.wmv *.mpeg *.mpg)")]
        onAccepted: root.controller.importVideoFiles(selectedFiles)
    }

    FileDialog {
        id: imageDialog
        title: qsTr("Importar imagens")
        fileMode: FileDialog.OpenFiles
        nameFilters: [qsTr("Imagens (*.jpg *.jpeg *.png *.webp *.bmp *.gif *.tif *.tiff *.heic)")]
        onAccepted: root.controller.importImageFiles(selectedFiles)
    }
    FileDialog {
        id: bibleImportDialog
        title: qsTr("Importar JSON HolyScreen legado")
        nameFilters: [qsTr("HolyScreen Bíblia JSON (*.json)")]
        onAccepted: root.controller.bibleContext.importBibleTranslation(selectedFile)
    }
    FolderDialog {
        id: bibleFolderDialog
        title: qsTr("Selecionar repositório, data/canonical ou pasta da tradução")
        onAccepted: root.controller.bibleContext.importBibleFolder(selectedFolder)
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
                    enabled: !root.controller.bibleContext.bibleImportRunning && bibleGitUrl.text.trim().length > 0
                    onClicked: {
                        if (root.controller.bibleContext.importBibleGit(bibleGitUrl.text))
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
                    enabled: !root.controller.bibleContext.bibleImportRunning && bibleZipUrl.text.trim().length > 0
                    onClicked: {
                        if (root.controller.bibleContext.importBibleZip(bibleZipUrl.text))
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
        onAccepted: root.controller.bibleContext.confirmBibleImportLicenses()
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
                text: root.controller.bibleContext.bibleImportLicenseWarning
                wrapMode: Text.WordWrap
                color: "#eff6ff"
                font.bold: true
            }
        }
    }
    Connections {
        target: root.controller
        function onBibleImportStateChanged() {
            if (root.controller.bibleContext.bibleImportRequiresLicenseConfirmation
                    && !root.controller.bibleContext.bibleImportRunning
                    && !bibleLicenseDialog.visible)
                bibleLicenseDialog.open()
        }
    }
    Dialog {
        id: bibleDialog
        title: qsTr("Bíblia")
        modal: true
        width: Math.min(root.width - 80, 940)
        height: Math.min(root.height - 80, 680)
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        standardButtons: Dialog.Close

        contentItem: ColumnLayout {
            spacing: 10
            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("TRADUÇÕES (ATÉ 3 SIMULTÂNEAS)"); color: "#8da0bc"; font.bold: true }
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("IMPORTAR PASTA")
                    enabled: !root.controller.bibleContext.bibleImportRunning
                    onClicked: bibleFolderDialog.open()
                }
                Button {
                    text: qsTr("GIT / ZIP")
                    enabled: !root.controller.bibleContext.bibleImportRunning
                    onClicked: bibleOnlineImportDialog.open()
                }
                Button {
                    text: qsTr("JSON LEGADO")
                    enabled: !root.controller.bibleContext.bibleImportRunning
                    onClicked: bibleImportDialog.open()
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                visible: root.controller.bibleContext.bibleImportRunning
                         || root.controller.bibleContext.bibleImportMessage.length > 0
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        text: root.controller.bibleContext.bibleImportMessage
                        color: root.controller.bibleContext.bibleImportRunning ? "#70e1a7" : "#b8c6dc"
                        elide: Text.ElideRight
                    }
                    Button {
                        text: qsTr("CANCELAR")
                        visible: root.controller.bibleContext.bibleImportRunning
                        onClicked: root.controller.bibleContext.cancelBibleImport()
                    }
                }
                ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 100
                    value: root.controller.bibleContext.bibleImportProgress
                    indeterminate: root.controller.bibleContext.bibleImportRunning
                                   && root.controller.bibleContext.bibleImportProgress === 0
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
                    model: root.controller.bibleContext.bibleTranslations
                    textRole: "displayName"; valueRole: "id"
                    currentIndex: root.translationIndex(model, root.controller.bibleContext.biblePrimaryTranslationId)
                    onActivated: root.controller.bibleContext.biblePrimaryTranslationId = currentValue
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: root.optionalBibleTranslations
                    textRole: "displayName"; valueRole: "id"
                    currentIndex: root.translationIndex(model, root.controller.bibleContext.bibleSecondaryTranslationId)
                    onActivated: root.controller.bibleContext.bibleSecondaryTranslationId = currentValue
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: root.optionalBibleTranslations
                    textRole: "displayName"; valueRole: "id"
                    currentIndex: root.translationIndex(model, root.controller.bibleContext.bibleTertiaryTranslationId)
                    onActivated: root.controller.bibleContext.bibleTertiaryTranslationId = currentValue
                }
            }
            RowLayout {
                Layout.fillWidth: true
                visible: root.controller.bibleContext.bibleTranslations.length > 0
                Label { text: qsTr("ORIGEM:"); color: "#8da0bc"; font.bold: true }
                ComboBox {
                    id: bibleManagedTranslation
                    Layout.fillWidth: true
                    model: root.controller.bibleContext.bibleTranslations
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
                    enabled: !root.controller.bibleContext.bibleImportRunning
                             && bibleManagedTranslation.currentIndex >= 0
                             && !!bibleManagedTranslation.model[bibleManagedTranslation.currentIndex].canUpdate
                    onClicked: root.controller.bibleContext.updateBibleTranslationFromSource(
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
                font.pixelSize: 11
            }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    Layout.fillWidth: true
                    placeholderText: qsTr("João 3:16, Jo 3 16 ou João 3.16")
                    text: root.controller.bibleContext.bibleReferenceInput
                    onTextEdited: root.controller.bibleContext.bibleReferenceInput = text
                    onAccepted: root.controller.bibleContext.searchBibleReference()
                }
                Button { text: qsTr("BUSCAR"); highlighted: true; onClicked: root.controller.bibleContext.searchBibleReference() }
            }
            Label {
                visible: root.controller.bibleContext.bibleTranslations.length === 0
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
                model: root.controller.bibleContext.bibleResults
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
                        Button { text: qsTr("APRESENTAR"); onClicked: root.controller.bibleContext.showBibleVerse(bibleResultDelegate.index) }
                    }
                }
                Label {
                    anchors.centerIn: parent
                    visible: bibleResultsList.count === 0 && root.controller.bibleContext.bibleTranslations.length > 0
                    text: qsTr("Digite uma referência para localizar os versículos")
                    color: "#64748b"
                }
            }
        }
    }
    FolderDialog {
        id: mediaFolderDialog
        title: qsTr("Adicionar pasta de mídia")
        onAccepted: root.controller.mediaContext.addMediaFolder(selectedFolder)
    }
    Dialog {
        id: mediaLibraryDialog
        title: qsTr("Biblioteca de pastas")
        modal: true
        width: Math.min(root.width - 80, 1000)
        height: Math.min(root.height - 80, 680)
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        standardButtons: Dialog.Close

        contentItem: ColumnLayout {
            spacing: 10
            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("PASTAS SELECIONADAS"); color: "#8da0bc"; font.bold: true }
                Item { Layout.fillWidth: true }
                Button { text: qsTr("+ PASTA"); onClicked: mediaFolderDialog.open() }
                Button { text: qsTr("ATUALIZAR"); onClicked: root.controller.mediaContext.rescanMediaFolders() }
            }
            ListView {
                id: mediaFoldersList
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(120, Math.max(42, contentHeight))
                clip: true
                spacing: 4
                model: root.controller.mediaContext.mediaFolders
                delegate: Rectangle {
                    id: mediaFolderDelegate
                    required property var modelData
                    width: ListView.view.width
                    height: 38
                    radius: 5
                    color: "#142137"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 6
                        Label { text: mediaFolderDelegate.modelData.exists ? "●" : "!"; color: mediaFolderDelegate.modelData.exists ? "#70e1a7" : "#ffba70" }
                        Label { Layout.fillWidth: true; text: mediaFolderDelegate.modelData.path; color: "#d9e5f5"; elide: Text.ElideMiddle }
                        ToolButton {
                            text: "×"
                            Accessible.name: qsTr("Remover pasta")
                            onClicked: root.controller.mediaContext.removeMediaFolder(mediaFolderDelegate.modelData.path)
                        }
                    }
                }
                Label {
                    anchors.centerIn: parent
                    visible: mediaFoldersList.count === 0
                    text: qsTr("Adicione uma ou mais pastas de áudio, vídeo ou imagens")
                    color: "#64748b"
                }
            }
            TabBar {
                id: mediaTypeTabs
                Layout.fillWidth: true
                TabButton { text: qsTr("ÁUDIOS (%1)").arg(root.controller.mediaContext.folderAudioFiles.length) }
                TabButton { text: qsTr("VÍDEOS (%1)").arg(root.controller.mediaContext.folderVideoFiles.length) }
                TabButton { text: qsTr("IMAGENS (%1)").arg(root.controller.mediaContext.folderImageFiles.length) }
            }
            TextField {
                Layout.fillWidth: true
                placeholderText: qsTr("Pesquisar por nome de arquivo")
                text: mediaTypeTabs.currentIndex === 0 ? root.controller.mediaContext.audioFileSearch
                      : mediaTypeTabs.currentIndex === 1 ? root.controller.mediaContext.videoFileSearch
                      : root.controller.mediaContext.imageFileSearch
                onTextEdited: {
                    if (mediaTypeTabs.currentIndex === 0) root.controller.mediaContext.audioFileSearch = text
                    else if (mediaTypeTabs.currentIndex === 1) root.controller.mediaContext.videoFileSearch = text
                    else root.controller.mediaContext.imageFileSearch = text
                }
            }
            ListView {
                id: folderMediaList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 4
                model: mediaTypeTabs.currentIndex === 0 ? root.controller.mediaContext.folderAudioFiles
                     : mediaTypeTabs.currentIndex === 1 ? root.controller.mediaContext.folderVideoFiles
                     : root.controller.mediaContext.folderImageFiles
                delegate: Rectangle {
                    id: folderMediaDelegate
                    required property var modelData
                    width: ListView.view.width
                    height: 46
                    radius: 6
                    color: "#142137"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 8
                        Label {
                            Layout.fillWidth: true
                            text: folderMediaDelegate.modelData.fileName
                            color: "#eff6ff"
                            elide: Text.ElideMiddle
                        }
                        Label {
                            text: folderMediaDelegate.modelData.folderPath
                            color: "#7185a3"
                            elide: Text.ElideMiddle
                            Layout.maximumWidth: 260
                        }
                        Button {
                            text: folderMediaDelegate.modelData.inPlaylist ? qsTr("NA PLAYLIST") : qsTr("+ PLAYLIST")
                            enabled: !folderMediaDelegate.modelData.inPlaylist
                            onClicked: root.controller.mediaContext.addCatalogFileToPlaylist(folderMediaDelegate.modelData.path)
                        }
                    }
                }
                Label {
                    anchors.centerIn: parent
                    visible: folderMediaList.count === 0
                    text: root.controller.mediaContext.mediaFolders.length === 0
                          ? qsTr("Nenhuma pasta selecionada")
                          : qsTr("Nenhum arquivo encontrado para esta pesquisa")
                    color: "#64748b"
                }
            }
        }
    }
    menuBar: MenuBar {
        visible: root.controller.debugEnabled
        Menu {
            title: qsTr("Debug")
            MenuItem {
                text: qsTr("Ativar modo de debug")
                checkable: true
                checked: root.controller.debugEnabled
                onTriggered: root.controller.debugEnabled = checked
            }
            MenuSeparator { }
            MenuItem {
                text: qsTr("Múltiplas saídas simuladas")
                checkable: true
                enabled: root.controller.debugEnabled
                checked: root.controller.debugSimulatedOutputs
                onTriggered: root.controller.debugSimulatedOutputs = checked
            }
            MenuItem {
                text: qsTr("Diagnósticos e benchmark")
                checkable: true
                enabled: root.controller.debugEnabled
                checked: root.controller.debugDiagnostics
                onTriggered: root.controller.debugDiagnostics = checked
            }
            MenuItem {
                text: qsTr("Registrar mensagens DEBUG")
                checkable: true
                enabled: root.controller.debugEnabled
                checked: root.controller.debugLogging
                onTriggered: root.controller.debugLogging = checked
            }
        }
    }

    header: OperatorHeader {
        controller: root.controller
        onOpenLive: liveDialog.open()
        onOpenEvents: eventsDialog.open()
        onOpenLibrary: mediaLibraryDialog.open()
        onOpenIntegrations: integrationsArea.open()
        onOpenAutomations: automationsArea.open()
        onOpenSettings: settingsDialog.open()
        onToggleFullScreen: root.visibility = root.visibility === Window.FullScreen
                            ? Window.Windowed : Window.FullScreen
    }
    BibleBrowser {
        id: bibleBrowser
        controller: root.controller.bibleContext
    }
    QuickBibleSearch {
        id: quickBibleSearch
        controller: root.controller.bibleContext
    }
    Connections {
        target: root.controller
        function onQuickBibleSearchRequested(initialText) {
            if (!quickBibleSearch.visible)
                quickBibleSearch.openWithText(initialText)
        }
    }

    Dashboard {
        id: operatorDashboard
        anchors.fill: parent
        controller: root.controller
        layoutSettings: operatorWindowSettings
        onOpenLibrary: mediaLibraryDialog.open()
        onOpenBible: bibleDialog.open()
        onOpenBibleBrowser: bibleBrowser.open()
        onImportAudio: audioDialog.open()
        onImportVideo: videoDialog.open()
        onImportImage: imageDialog.open()
    }

    Component.onCompleted: {
        width = Math.max(minimumWidth, operatorWindowSettings.savedWidth)
        height = Math.max(minimumHeight, operatorWindowSettings.savedHeight)
    }

    DropArea {
        anchors.fill: parent
        onDropped: function(drop) {
            if (!drop.urls || drop.urls.length === 0)
                return
            const audio = []
            const video = []
            const images = []
            for (let index = 0; index < drop.urls.length; ++index) {
                const value = drop.urls[index].toString().toLowerCase()
                if (value.match(/\.(mp3|wav|flac|m4a|aac|ogg|opus|wma|aiff|aif)$/))
                    audio.push(drop.urls[index])
                else if (value.match(/\.(mp4|mov|m4v|mkv|webm|avi|wmv|mpeg|mpg)$/))
                    video.push(drop.urls[index])
                else if (value.match(/\.(jpg|jpeg|png|webp|bmp|gif|tif|tiff|heic)$/))
                    images.push(drop.urls[index])
            }
            if (audio.length > 0) root.controller.importAudioFiles(audio)
            if (video.length > 0) root.controller.importVideoFiles(video)
            if (images.length > 0) root.controller.importImageFiles(images)
        }
    }

    Shortcut { sequence: root.controller.shortcuts.next; enabled: root.controller.textVisible; onActivated: root.controller.nextTextSlide() }
    Shortcut { sequence: root.controller.shortcuts.previous; enabled: root.controller.textVisible; onActivated: root.controller.previousTextSlide() }
    Shortcut { sequence: "Home"; enabled: root.controller.textVisible; onActivated: root.controller.firstTextSlide() }
    Shortcut { sequence: "End"; enabled: root.controller.textVisible; onActivated: root.controller.lastTextSlide() }
    Shortcut { sequence: root.controller.shortcuts.stop; enabled: root.controller.textVisible; onActivated: root.controller.stopTextPresentation() }
    Shortcut { sequence: StandardKey.Undo; enabled: root.controller.canUndo; onActivated: root.controller.undo() }
    Shortcut { sequence: StandardKey.Redo; enabled: root.controller.canRedo; onActivated: root.controller.redo() }
    Shortcut { sequence: root.controller.shortcuts.blackout; onActivated: root.controller.outputContext.blackout = !root.controller.outputContext.blackout }
    Shortcut { sequence: root.controller.shortcuts.quickBible; onActivated: quickBibleSearch.openWithText("") }
    Shortcut { sequence: "Ctrl+Shift+B"; onActivated: root.controller.maintenanceContext.createBackup() }
    Shortcut { sequence: "F5"; onActivated: root.controller.maintenanceContext.checkForUpdates() }
    Shortcut { sequence: "Ctrl+Shift+O"; onActivated: { root.show(); root.raise(); root.requestActivate() } }
}
