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
                                                     .concat(root.controller.bibleTranslations)
    readonly property var outputBibleTranslations: [{"id":"", "displayName":qsTr("Bíblia: composição padrão")}]
                                                   .concat(root.controller.bibleTranslations)
    readonly property var outputRoles: [{"id":"audience", "displayName":qsTr("Saída: público")},
                                        {"id":"stage", "displayName":qsTr("Saída: palco")},
                                        {"id":"broadcast", "displayName":qsTr("Saída: transmissão")}]
    readonly property int previewOutputCount: root.controller.debugEnabled
                                              && root.controller.debugSimulatedOutputs
                                              ? root.controller.simulatedOutputCount : 1

    Settings {
        id: operatorWindowSettings
        category: "OperatorWindow"
        property int savedWidth: 1360
        property int savedHeight: 820
        property var dashboardHorizontalState
        property var dashboardVerticalState
    }

    function formatDuration(milliseconds) {
        const totalSeconds = Math.max(0, Math.floor(milliseconds / 1000))
        const minutes = Math.floor(totalSeconds / 60)
        const seconds = totalSeconds % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    function mediaStateLabel(state) {
        switch (state) {
        case "loading": return qsTr("CARREGANDO")
        case "ready": return qsTr("PRONTO")
        case "playing": return qsTr("REPRODUZINDO")
        case "paused": return qsTr("PAUSADO")
        case "buffering": return qsTr("CARREGANDO BUFFER")
        case "error": return qsTr("ERRO")
        default: return qsTr("PARADO")
        }
    }

    function translationIndex(model, translationId) {
        for (let index = 0; index < model.length; ++index) {
            if (model[index].id === translationId)
                return index
        }
        return 0
    }

    function valueIndex(model, id) {
        for (let index = 0; index < model.length; ++index) {
            if (model[index].id === id)
                return index
        }
        return 0
    }

    Instantiator {
        model: root.controller.outputWindows
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

    function playFavorite(path) {
        const mediaId = root.controller.addCatalogFileToPlaylist(path)
        if (mediaId.length > 0)
            root.controller.playMedia(mediaId)
    }
    IntegrationsArea {
        id: integrationsArea
        controller: root.controller.integrationContext
    }

    AutomationsArea {
        id: automationsArea
        controller: root.controller.automationContext
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
        onAccepted: root.controller.importBibleTranslation(selectedFile)
    }
    FolderDialog {
        id: bibleFolderDialog
        title: qsTr("Selecionar repositório, data/canonical ou pasta da tradução")
        onAccepted: root.controller.importBibleFolder(selectedFolder)
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
                    enabled: !root.controller.bibleImportRunning && bibleGitUrl.text.trim().length > 0
                    onClicked: {
                        if (root.controller.importBibleGit(bibleGitUrl.text))
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
                    enabled: !root.controller.bibleImportRunning && bibleZipUrl.text.trim().length > 0
                    onClicked: {
                        if (root.controller.importBibleZip(bibleZipUrl.text))
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
        onAccepted: root.controller.confirmBibleImportLicenses()
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
                text: root.controller.bibleImportLicenseWarning
                wrapMode: Text.WordWrap
                color: "#eff6ff"
                font.bold: true
            }
        }
    }
    Connections {
        target: root.controller
        function onBibleImportStateChanged() {
            if (root.controller.bibleImportRequiresLicenseConfirmation
                    && !root.controller.bibleImportRunning
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
                    enabled: !root.controller.bibleImportRunning
                    onClicked: bibleFolderDialog.open()
                }
                Button {
                    text: qsTr("GIT / ZIP")
                    enabled: !root.controller.bibleImportRunning
                    onClicked: bibleOnlineImportDialog.open()
                }
                Button {
                    text: qsTr("JSON LEGADO")
                    enabled: !root.controller.bibleImportRunning
                    onClicked: bibleImportDialog.open()
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                visible: root.controller.bibleImportRunning
                         || root.controller.bibleImportMessage.length > 0
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        text: root.controller.bibleImportMessage
                        color: root.controller.bibleImportRunning ? "#70e1a7" : "#b8c6dc"
                        elide: Text.ElideRight
                    }
                    Button {
                        text: qsTr("CANCELAR")
                        visible: root.controller.bibleImportRunning
                        onClicked: root.controller.cancelBibleImport()
                    }
                }
                ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 100
                    value: root.controller.bibleImportProgress
                    indeterminate: root.controller.bibleImportRunning
                                   && root.controller.bibleImportProgress === 0
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
                    model: root.controller.bibleTranslations
                    textRole: "displayName"; valueRole: "id"
                    currentIndex: root.translationIndex(model, root.controller.biblePrimaryTranslationId)
                    onActivated: root.controller.biblePrimaryTranslationId = currentValue
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: root.optionalBibleTranslations
                    textRole: "displayName"; valueRole: "id"
                    currentIndex: root.translationIndex(model, root.controller.bibleSecondaryTranslationId)
                    onActivated: root.controller.bibleSecondaryTranslationId = currentValue
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: root.optionalBibleTranslations
                    textRole: "displayName"; valueRole: "id"
                    currentIndex: root.translationIndex(model, root.controller.bibleTertiaryTranslationId)
                    onActivated: root.controller.bibleTertiaryTranslationId = currentValue
                }
            }
            RowLayout {
                Layout.fillWidth: true
                visible: root.controller.bibleTranslations.length > 0
                Label { text: qsTr("ORIGEM:"); color: "#8da0bc"; font.bold: true }
                ComboBox {
                    id: bibleManagedTranslation
                    Layout.fillWidth: true
                    model: root.controller.bibleTranslations
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
                    enabled: !root.controller.bibleImportRunning
                             && bibleManagedTranslation.currentIndex >= 0
                             && !!bibleManagedTranslation.model[bibleManagedTranslation.currentIndex].canUpdate
                    onClicked: root.controller.updateBibleTranslationFromSource(
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
                    text: root.controller.bibleReferenceInput
                    onTextEdited: root.controller.bibleReferenceInput = text
                    onAccepted: root.controller.searchBibleReference()
                }
                Button { text: qsTr("BUSCAR"); highlighted: true; onClicked: root.controller.searchBibleReference() }
            }
            Label {
                visible: root.controller.bibleTranslations.length === 0
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
                model: root.controller.bibleResults
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
                        Button { text: qsTr("APRESENTAR"); onClicked: root.controller.showBibleVerse(bibleResultDelegate.index) }
                    }
                }
                Label {
                    anchors.centerIn: parent
                    visible: bibleResultsList.count === 0 && root.controller.bibleTranslations.length > 0
                    text: qsTr("Digite uma referência para localizar os versículos")
                    color: "#64748b"
                }
            }
        }
    }
    FolderDialog {
        id: mediaFolderDialog
        title: qsTr("Adicionar pasta de mídia")
        onAccepted: root.controller.addMediaFolder(selectedFolder)
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
                Button { text: qsTr("ATUALIZAR"); onClicked: root.controller.rescanMediaFolders() }
            }
            ListView {
                id: mediaFoldersList
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(120, Math.max(42, contentHeight))
                clip: true
                spacing: 4
                model: root.controller.mediaFolders
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
                            onClicked: root.controller.removeMediaFolder(mediaFolderDelegate.modelData.path)
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
                TabButton { text: qsTr("ÁUDIOS (%1)").arg(root.controller.folderAudioFiles.length) }
                TabButton { text: qsTr("VÍDEOS (%1)").arg(root.controller.folderVideoFiles.length) }
                TabButton { text: qsTr("IMAGENS (%1)").arg(root.controller.folderImageFiles.length) }
            }
            TextField {
                Layout.fillWidth: true
                placeholderText: qsTr("Pesquisar por nome de arquivo")
                text: mediaTypeTabs.currentIndex === 0 ? root.controller.audioFileSearch
                      : mediaTypeTabs.currentIndex === 1 ? root.controller.videoFileSearch
                      : root.controller.imageFileSearch
                onTextEdited: {
                    if (mediaTypeTabs.currentIndex === 0) root.controller.audioFileSearch = text
                    else if (mediaTypeTabs.currentIndex === 1) root.controller.videoFileSearch = text
                    else root.controller.imageFileSearch = text
                }
            }
            ListView {
                id: folderMediaList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 4
                model: mediaTypeTabs.currentIndex === 0 ? root.controller.folderAudioFiles
                     : mediaTypeTabs.currentIndex === 1 ? root.controller.folderVideoFiles
                     : root.controller.folderImageFiles
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
                            onClicked: root.controller.addCatalogFileToPlaylist(folderMediaDelegate.modelData.path)
                        }
                    }
                }
                Label {
                    anchors.centerIn: parent
                    visible: folderMediaList.count === 0
                    text: root.controller.mediaFolders.length === 0
                          ? qsTr("Nenhuma pasta selecionada")
                          : qsTr("Nenhum arquivo encontrado para esta pesquisa")
                    color: "#64748b"
                }
            }
        }
    }
    FileDialog {
        id: themeBackgroundDialog
        title: qsTr("Selecionar fundo do tema")
        nameFilters: [qsTr("Imagens (*.jpg *.jpeg *.png *.webp *.bmp)")]
        onAccepted: root.controller.updateTheme({backgroundType: 2, backgroundImage: selectedFile.toString()})
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

    header: ToolBar {
        height: 92
        background: Rectangle { color: "#15191d"; border.color: "#353b40" }
        ColumnLayout {
            anchors.fill: parent
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                Layout.leftMargin: 18
                Layout.rightMargin: 14
                spacing: 12
                Label {
                    text: qsTr("HolyScreen")
                    color: "#f2f4f5"
                    font.bold: true
                    font.pixelSize: 15
                }
                ToolButton { text: qsTr("Live"); onClicked: liveDialog.open() }
                ToolButton { text: qsTr("Prévia") }
                ToolButton { text: qsTr("Agenda") }
                ToolButton { text: qsTr("Biblioteca"); font.bold: true; onClicked: mediaLibraryDialog.open() }
                ToolButton { text: qsTr("Integrações"); onClicked: integrationsArea.open() }
                ToolButton {
                    text: qsTr("Automações")
                    onClicked: automationsArea.open()
                    ToolTip.visible: hovered && !root.controller.automationsEnabled
                    ToolTip.text: qsTr("Automações pausadas")
                }
                Item { Layout.fillWidth: true }
                ToolButton {
                    text: "⚙"
                    Accessible.name: qsTr("Configurações")
                    onClicked: settingsDialog.open()
                }
                ToolButton {
                    text: "⛶"
                    Accessible.name: qsTr("Alternar tela cheia")
                    onClicked: root.visibility = root.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen
                }
                Button { text: root.controller.outputContext.blackout ? qsTr("Restaurar") : qsTr("Ao vivo"); highlighted: true; onClicked: root.controller.outputContext.blackout = false }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                color: "#1d2227"
                border.color: "#353b40"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 18
                    anchors.rightMargin: 14
                    spacing: 10
                    Label {
                        text: qsTr("★  FAVORITOS")
                        color: "#c7d2fe"
                        font.bold: true
                        font.pixelSize: 11
                    }
                    Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; Layout.topMargin: 8; Layout.bottomMargin: 8; color: "#41484e" }
                    ListView {
                        id: favoriteMediaList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        orientation: ListView.Horizontal
                        spacing: 6
                        clip: true
                        model: root.controller.favoriteMedia
                        delegate: Button {
                            required property var modelData
                            height: 32
                            width: Math.min(220, Math.max(110, implicitWidth))
                            y: (favoriteMediaList.height - height) / 2
                            text: (modelData.type === "video" ? "▶  "
                                  : modelData.type === "image" ? "▧  " : "♫  ")
                                  + (modelData.fileName || modelData.title || qsTr("Sem título"))
                            font.pixelSize: 11
                            onClicked: root.playFavorite(modelData.path)
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Reproduzir %1").arg(modelData.fileName || modelData.title)
                        }
                        Label {
                            anchors.centerIn: parent
                            visible: favoriteMediaList.count === 0
                            text: qsTr("Clique com o botão direito em uma mídia para adicioná-la")
                            color: "#8d979f"
                            font.pixelSize: 11
                        }
                    }
                }
            }
        }
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

    RowLayout {
        visible: false
        anchors.fill: parent
        anchors.margins: 18
        spacing: 16

        Rectangle {
            Layout.preferredWidth: 290
            Layout.fillHeight: true
            color: "#101a2d"
            radius: 12
            border.color: "#21304a"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12
                Label { text: qsTr("TELAS"); color: "#e6edf7"; font.bold: true; font.pixelSize: 15 }
                Label {
                    text: qsTr("Selecione até cinco saídas e defina público ou palco.")
                    color: "#8da0bc"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    font.pixelSize: 12
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        Repeater {
                            model: root.controller.screens
                            delegate: Rectangle {
                                id: screenDelegate
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: modelData.selected
                                                ? (root.controller.bibleTranslations.length > 0 ? 176 : 146)
                                                : 80
                                radius: 8
                                color: modelData.selected ? "#18345a" : "#152137"
                                border.color: modelData.selected ? "#3b82f6" : "#263852"
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    CheckBox {
                                        checked: screenDelegate.modelData.selected
                                        enabled: !screenDelegate.modelData.primary
                                        // `toggled` also reacts to model-driven checked changes
                                        // while delegates are rebuilt. Only user clicks should
                                        // mutate the configured output list.
                                        onClicked: root.controller.toggleScreen(screenDelegate.modelData.id, checked)
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        Label {
                                            text: screenDelegate.modelData.name
                                            color: "#eff6ff"
                                            font.bold: true
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                        Label {
                                            text: screenDelegate.modelData.primary ? qsTr("OPERADOR • tela principal")
                                                  : !screenDelegate.modelData.selected ? qsTr("DISPONÍVEL")
                                                  : screenDelegate.modelData.role === "stage" ? qsTr("PALCO")
                                                  : screenDelegate.modelData.role === "broadcast" ? qsTr("TRANSMISSÃO")
                                                  : screenDelegate.modelData.role === "confidence" ? qsTr("CONFERÊNCIA")
                                                  : screenDelegate.modelData.role === "custom" ? qsTr("PERSONALIZADA")
                                                  : qsTr("PÚBLICO")
                                            color: "#8da0bc"
                                            font.pixelSize: 11
                                        }
                                        ComboBox {
                                            visible: !screenDelegate.modelData.primary && screenDelegate.modelData.selected
                                            Layout.fillWidth: true
                                            model: root.outputRoles
                                            textRole: "displayName"
                                            valueRole: "id"
                                            currentIndex: root.valueIndex(model, screenDelegate.modelData.role)
                                            onActivated: root.controller.setOutputRole(screenDelegate.modelData.id, currentValue)
                                        }
                                        CheckBox {
                                            visible: !screenDelegate.modelData.primary && screenDelegate.modelData.selected
                                            text: qsTr("Exibir mídia nesta tela")
                                            checked: screenDelegate.modelData.mediaEnabled
                                            onClicked: root.controller.setOutputMediaEnabled(screenDelegate.modelData.id, checked)
                                        }
                                        ComboBox {
                                            visible: !screenDelegate.modelData.primary && screenDelegate.modelData.selected
                                                     && root.controller.bibleTranslations.length > 0
                                            Layout.fillWidth: true
                                            model: root.outputBibleTranslations
                                            textRole: "displayName"
                                            valueRole: "id"
                                            currentIndex: root.translationIndex(model, screenDelegate.modelData.bibleTranslationId)
                                            onActivated: root.controller.setOutputBibleTranslation(screenDelegate.modelData.id, currentValue)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                Button {
                    text: qsTr("ATIVAR TODAS")
                    Layout.fillWidth: true
                    onClicked: root.controller.enableAllScreens()
                }
                Button {
                    text: qsTr("IDENTIFICAR TELAS")
                    Layout.fillWidth: true
                    onClicked: root.controller.identifyScreens()
                }
                Label {
                    visible: root.controller.statusMessage.length > 0
                    text: root.controller.statusMessage
                    color: "#ffba70"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    font.pixelSize: 11
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#101a2d"
            radius: 12
            border.color: "#21304a"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 14
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("PRÉVIA DE SAÍDA"); color: "#e6edf7"; font.bold: true; font.pixelSize: 15 }
                    Item { Layout.fillWidth: true }
                    Label { text: qsTr("Wallpaper + Relógio"); color: "#8da0bc"; font.pixelSize: 12 }
                }

                GridLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: root.previewOutputCount > 1 ? 2 : 1
                    columnSpacing: 14
                    rowSpacing: 14

                    Repeater {
                        model: root.previewOutputCount
                        delegate: SimulatedOutput {
                            controller: root.controller
                            required property int index
                            Layout.minimumWidth: 220
                            Layout.minimumHeight: root.previewOutputCount > 4 ? 80
                                                  : root.previewOutputCount > 2 ? 110 : 160
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            outputLabel: root.previewOutputCount > 1
                                         ? qsTr("SIMULAÇÃO %1").arg(index + 1)
                                         : qsTr("PRÉVIA")
                            identifier: index + 1
                            wallpaper: root.controller.wallpaperColor
                            wallpaperSource: root.controller.wallpaperSource
                            wallpaperFit: root.controller.wallpaperFit
                            showClock: root.controller.clockVisible
                            clockText: root.controller.clockText
                            clockPosition: root.controller.clockPosition
                            clockFamily: root.controller.clockFontFamily
                            clockColor: root.controller.clockColor
                            isBlackout: root.controller.outputContext.blackout
                            identifyVisible: root.controller.identifyVisible
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 250
                    color: "#0c1628"
                    radius: 10
                    border.color: "#263852"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 14

                        ColumnLayout {
                            Layout.preferredWidth: 310
                            Layout.fillHeight: true
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: qsTr("PLAYLIST MULTIMÍDIA"); color: "#e6edf7"; font.bold: true; font.pixelSize: 12 }
                                Item { Layout.fillWidth: true }
                                Button { text: qsTr("PASTAS"); onClicked: mediaLibraryDialog.open() }
                                Button { text: qsTr("+ ÁUDIO"); onClicked: audioDialog.open() }
                                Button { text: qsTr("+ VÍDEO"); onClicked: videoDialog.open() }
                            }
                            ListView {
                                id: mediaList
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                spacing: 4
                                model: root.controller.mediaPlaylist
                                delegate: Rectangle {
                                    id: mediaDelegate
                                    required property var modelData
                                    required property int index
                                    width: ListView.view.width
                                    height: 44
                                    radius: 6
                                    color: root.controller.currentMediaId === modelData.id ? "#18345a" : "#142137"
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 10
                                        anchors.rightMargin: 6
                                        Label {
                                            Layout.fillWidth: true
                                            text: mediaDelegate.modelData.title
                                            color: "#eff6ff"
                                            elide: Text.ElideRight
                                        }
                                        Label { text: mediaDelegate.modelData.typeLabel; color: mediaDelegate.modelData.type === "video" ? "#c4a7ff" : "#70e1a7"; font.pixelSize: 9; font.bold: true }
                                        Label { text: root.formatDuration(mediaDelegate.modelData.durationMs); color: "#8da0bc"; font.pixelSize: 11 }
                                        ToolButton {
                                            text: "↑"
                                            Accessible.name: qsTr("Mover mídia para cima")
                                            enabled: mediaDelegate.index > 0
                                            onClicked: root.controller.moveMedia(mediaDelegate.modelData.id, mediaDelegate.index - 1)
                                        }
                                        ToolButton {
                                            text: "↓"
                                            Accessible.name: qsTr("Mover mídia para baixo")
                                            enabled: mediaDelegate.index + 1 < mediaList.count
                                            onClicked: root.controller.moveMedia(mediaDelegate.modelData.id, mediaDelegate.index + 1)
                                        }
                                        ToolButton {
                                            text: "×"
                                            Accessible.name: qsTr("Remover mídia da playlist")
                                            onClicked: root.controller.removeMedia(mediaDelegate.modelData.id)
                                        }
                                    }
                                    TapHandler {
                                        acceptedButtons: Qt.LeftButton
                                        onDoubleTapped: root.controller.playMedia(mediaDelegate.modelData.id)
                                    }
                                }
                                Label {
                                    anchors.centerIn: parent
                                    visible: mediaList.count === 0
                                    text: qsTr("Importe ou arraste áudios e vídeos")
                                    color: "#64748b"
                                }
                            }
                        }

                        Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#24334b" }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Label {
                                Layout.fillWidth: true
                                text: root.controller.currentMediaTitle.length > 0
                                      ? root.controller.currentMediaTitle : qsTr("Nenhuma mídia selecionada")
                                color: "#eff6ff"
                                font.bold: true
                                font.pixelSize: 14
                                elide: Text.ElideRight
                            }
                            Label {
                                text: (root.controller.currentMediaType.length > 0
                                       ? root.controller.currentMediaType.toUpperCase() + " · " : "")
                                      + root.mediaStateLabel(root.controller.mediaState)
                                color: root.controller.mediaState === "playing" ? "#70e1a7" : "#8da0bc"
                                font.pixelSize: 10
                                font.bold: true
                            }
                            Slider {
                                Layout.fillWidth: true
                                from: 0
                                to: Math.max(1, root.controller.mediaDurationMs)
                                value: root.controller.mediaPositionMs
                                onMoved: root.controller.seekMedia(value)
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: root.formatDuration(root.controller.mediaPositionMs); color: "#8da0bc" }
                                Item { Layout.fillWidth: true }
                                Label { text: root.formatDuration(root.controller.mediaDurationMs); color: "#8da0bc" }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Button { text: qsTr("ANTERIOR"); onClicked: root.controller.mediaContext.previousMedia() }
                                Button {
                                    text: root.controller.mediaState === "playing" ? qsTr("PAUSAR") : qsTr("TOCAR")
                                    highlighted: true
                                    onClicked: {
                                        if (root.controller.currentMediaId.length === 0 && mediaList.count > 0)
                                            root.controller.playMedia(root.controller.mediaPlaylist[0].id)
                                        else
                                            root.controller.mediaContext.toggleMediaPause()
                                    }
                                }
                                Button { text: qsTr("PARAR"); onClicked: root.controller.mediaContext.stopMedia() }
                                Button { text: qsTr("PRÓXIMO"); onClicked: root.controller.mediaContext.nextMedia() }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: qsTr("VOLUME"); color: "#8da0bc"; font.pixelSize: 10 }
                                Slider {
                                    Layout.fillWidth: true
                                    from: 0; to: 1; stepSize: 0.01
                                    value: root.controller.mediaVolume
                                    onMoved: root.controller.mediaVolume = value
                                }
                                ComboBox {
                                    model: [qsTr("Não repetir"), qsTr("Repetir uma"), qsTr("Repetir playlist")]
                                    currentIndex: root.controller.mediaRepeatMode === "one" ? 1
                                                  : root.controller.mediaRepeatMode === "all" ? 2 : 0
                                    onActivated: root.controller.mediaRepeatMode = currentIndex === 1 ? "one"
                                                 : currentIndex === 2 ? "all" : "off"
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 310
            Layout.fillHeight: true
            color: "#101a2d"
            radius: 12
            border.color: "#21304a"

            ScrollView {
                anchors.fill: parent
                anchors.margins: 16
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 12
                    Label { text: qsTr("CONTROLES"); color: "#e6edf7"; font.bold: true; font.pixelSize: 15 }
                    Label { text: qsTr("WALLPAPER"); color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
                    GridLayout {
                        columns: 4
                        Repeater {
                            model: ["#000000", "#15243a", "#1c355e", "#3b1d4f", "#20443c", "#43212c", "#7c5e10", "#113b5c"]
                            delegate: Rectangle {
                                id: wallpaperSwatch
                                required property string modelData
                                width: 48; height: 34; radius: 7; color: modelData
                                border.width: root.controller.wallpaperColor === modelData ? 3 : 1
                                border.color: root.controller.wallpaperColor === modelData ? "#f8fafc" : "#3b4c66"
                                TapHandler { onTapped: root.controller.wallpaperColor = wallpaperSwatch.modelData }
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: qsTr("ESCOLHER IMAGEM"); Layout.fillWidth: true; onClicked: wallpaperDialog.open() }
                        Button { text: qsTr("LIMPAR"); onClicked: root.controller.wallpaperSource = "" }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["cover", "contain", "stretch", "center"]
                        currentIndex: Math.max(0, model.indexOf(root.controller.wallpaperFit))
                        onActivated: root.controller.wallpaperFit = currentText
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { text: qsTr("RELÓGIO"); color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
                    CheckBox {
                        text: qsTr("Exibir relógio")
                        checked: root.controller.clockVisible
                        onToggled: root.controller.clockVisible = checked
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["bottomRight", "topRight", "bottomLeft", "topLeft"]
                        currentIndex: Math.max(0, model.indexOf(root.controller.clockPosition))
                        onActivated: root.controller.clockPosition = currentText
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["24h", "24h-seconds", "12h"]
                        currentIndex: Math.max(0, model.indexOf(root.controller.clockFormat))
                        onActivated: root.controller.clockFormat = currentText
                    }
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: qsTr("Fonte")
                        text: root.controller.clockFontFamily
                        onEditingFinished: root.controller.clockFontFamily = text
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Tamanho"); color: "#c8d5e8" }
                        SpinBox {
                            from: 16; to: 240
                            value: root.controller.clockFontSize
                            onValueModified: root.controller.clockFontSize = value
                        }
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { text: qsTr("IMAGENS"); color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: qsTr("IMPORTAR"); Layout.fillWidth: true; onClicked: imageDialog.open() }
                        Button {
                            text: qsTr("REMOVER")
                            enabled: imagePicker.currentValue !== undefined && imagePicker.currentValue.length > 0
                            onClicked: root.controller.removeImage(imagePicker.currentValue)
                        }
                    }
                    ComboBox {
                        id: imagePicker
                        Layout.fillWidth: true
                        model: root.controller.imageLibrary
                        textRole: "title"
                        valueRole: "id"
                        displayText: root.controller.currentImageTitle.length > 0
                                     ? root.controller.currentImageTitle
                                     : (currentText.length > 0 ? currentText : qsTr("Nenhuma imagem"))
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        ToolButton {
                            text: "↑"
                            Accessible.name: qsTr("Mover imagem para cima")
                            enabled: imagePicker.currentIndex > 0
                            onClicked: root.controller.moveImage(imagePicker.currentValue, imagePicker.currentIndex - 1)
                        }
                        ToolButton {
                            text: "↓"
                            Accessible.name: qsTr("Mover imagem para baixo")
                            enabled: imagePicker.currentIndex >= 0 && imagePicker.currentIndex + 1 < imagePicker.count
                            onClicked: root.controller.moveImage(imagePicker.currentValue, imagePicker.currentIndex + 1)
                        }
                        Button {
                            text: qsTr("EXIBIR")
                            Layout.fillWidth: true
                            highlighted: true
                            enabled: imagePicker.currentValue !== undefined && imagePicker.currentValue.length > 0
                            onClicked: root.controller.showImage(imagePicker.currentValue)
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: qsTr("ANTERIOR"); Layout.fillWidth: true; onClicked: root.controller.previousImage() }
                        Button { text: qsTr("PRÓXIMA"); Layout.fillWidth: true; onClicked: root.controller.nextImage() }
                        Button { text: qsTr("PARAR"); onClicked: root.controller.stopImage() }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["contain", "cover", "stretch", "center"]
                            currentIndex: Math.max(0, model.indexOf(root.controller.imageFit))
                            onActivated: root.controller.imageFit = currentText
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["fade", "none"]
                            currentIndex: Math.max(0, model.indexOf(root.controller.imageTransition))
                            onActivated: root.controller.imageTransition = currentText
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        CheckBox {
                            text: qsTr("Autoplay")
                            checked: root.controller.imageAutoplay
                            onToggled: root.controller.imageAutoplay = checked
                        }
                        Label { text: qsTr("segundos"); color: "#8da0bc"; font.pixelSize: 10 }
                        SpinBox {
                            from: 1; to: 3600
                            value: Math.round(root.controller.imageIntervalMs / 1000)
                            onValueModified: root.controller.imageIntervalMs = value * 1000
                        }
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { text: qsTr("APRESENTAÇÕES DE TEXTO"); color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField { id: newPresentationTitle; Layout.fillWidth: true; placeholderText: qsTr("Título da apresentação") }
                        Button {
                            text: qsTr("NOVA")
                            onClicked: {
                                if (newPresentationTitle.text.trim().length > 0) {
                                    root.controller.createTextPresentation(newPresentationTitle.text)
                                    newPresentationTitle.clear()
                                }
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        ComboBox {
                            id: presentationPicker
                            Layout.fillWidth: true
                            model: root.controller.textPresentations
                            textRole: "title"; valueRole: "id"
                            onActivated: root.controller.selectTextPresentation(currentValue)
                        }
                        Button {
                            text: qsTr("EXCLUIR")
                            enabled: root.controller.currentPresentationId.length > 0
                            onClicked: root.controller.deleteTextPresentation(root.controller.currentPresentationId)
                        }
                    }
                    ComboBox {
                        id: slidePicker
                        Layout.fillWidth: true
                        model: root.controller.textSlides
                        textRole: "label"
                        currentIndex: Math.max(0, root.controller.currentSlideIndex)
                        onActivated: root.controller.showTextSlide(currentIndex)
                    }
                    TextField {
                        id: slideLabelEditor
                        Layout.fillWidth: true
                        placeholderText: qsTr("Rótulo do slide")
                        text: root.controller.currentSlideLabel
                    }
                    TextArea {
                        id: slideTextEditor
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        placeholderText: qsTr("Texto do slide")
                        text: root.controller.currentSlideText
                        wrapMode: TextEdit.Wrap
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            text: qsTr("SALVAR")
                            Layout.fillWidth: true
                            enabled: root.controller.currentSlideId.length > 0
                            onClicked: root.controller.updateTextSlide(root.controller.currentSlideId, slideLabelEditor.text, slideTextEditor.text)
                        }
                        Button {
                            text: qsTr("+ SLIDE")
                            enabled: root.controller.currentPresentationId.length > 0
                            onClicked: root.controller.addTextSlide(String(root.controller.textSlides.length + 1), qsTr("Novo slide"))
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: qsTr("DUPLICAR"); enabled: root.controller.currentSlideId.length > 0; onClicked: root.controller.duplicateTextSlide(root.controller.currentSlideId) }
                        Button { text: qsTr("DIVIDIR"); enabled: root.controller.currentSlideId.length > 0; onClicked: root.controller.splitTextSlide(root.controller.currentSlideId, slideTextEditor.cursorPosition) }
                        Button { text: qsTr("EXCLUIR"); enabled: root.controller.currentSlideId.length > 0; onClicked: root.controller.removeTextSlide(root.controller.currentSlideId) }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        ToolButton {
                            text: "↑"
                            Accessible.name: qsTr("Mover slide para cima")
                            enabled: slidePicker.currentIndex > 0
                            onClicked: root.controller.moveTextSlide(root.controller.currentSlideId, slidePicker.currentIndex - 1)
                        }
                        ToolButton {
                            text: "↓"
                            Accessible.name: qsTr("Mover slide para baixo")
                            enabled: slidePicker.currentIndex + 1 < slidePicker.count
                            onClicked: root.controller.moveTextSlide(root.controller.currentSlideId, slidePicker.currentIndex + 1)
                        }
                        Button { text: qsTr("ANTERIOR"); Layout.fillWidth: true; onClicked: root.controller.previousTextSlide() }
                        Button { text: qsTr("EXIBIR"); highlighted: true; enabled: slidePicker.currentIndex >= 0; onClicked: root.controller.showTextSlide(slidePicker.currentIndex) }
                        Button { text: qsTr("PRÓXIMO"); Layout.fillWidth: true; onClicked: root.controller.nextTextSlide() }
                    }
                    Button { text: qsTr("VOLTAR AO WALLPAPER"); Layout.fillWidth: true; onClicked: root.controller.stopTextPresentation() }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { text: qsTr("TEMAS"); color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField { id: newThemeName; Layout.fillWidth: true; placeholderText: qsTr("Nome do tema") }
                        Button { text: qsTr("NOVO"); onClicked: { if(newThemeName.text.trim().length>0){root.controller.createTheme(newThemeName.text);newThemeName.clear()} } }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        ComboBox { id: themePicker; Layout.fillWidth: true; model: root.controller.themes; textRole:"name"; valueRole:"id"; onActivated: root.controller.applyTheme(currentValue) }
                        Button { text:qsTr("APLICAR"); enabled:themePicker.currentValue!==undefined; onClicked:root.controller.applyTheme(themePicker.currentValue) }
                        Button { text:qsTr("EXCLUIR"); enabled:root.controller.themes.length>1; onClicked:root.controller.deleteTheme(themePicker.currentValue) }
                    }
                    TextField { Layout.fillWidth:true; placeholderText:qsTr("Família da fonte"); text:root.controller.activeTheme.fontFamily || ""; onEditingFinished:root.controller.updateTheme({fontFamily:text}) }
                    RowLayout {
                        Layout.fillWidth:true
                        Label { text:qsTr("Fonte"); color:"#8da0bc" }
                        SpinBox { from:28;to:240;value:root.controller.activeTheme.fontSize || 72; onValueModified:root.controller.updateTheme({fontSize:value}) }
                        Label { text:qsTr("Margem"); color:"#8da0bc" }
                        SpinBox { from:0;to:400;value:root.controller.activeTheme.margin || 64; onValueModified:root.controller.updateTheme({margin:value}) }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        TextField { Layout.fillWidth:true; placeholderText:qsTr("Cor do texto"); text:root.controller.activeTheme.textColor || "#ffffff"; onEditingFinished:root.controller.updateTheme({textColor:text}) }
                        TextField { Layout.fillWidth:true; placeholderText:qsTr("Cor de fundo"); text:root.controller.activeTheme.backgroundColor || "#000000"; onEditingFinished:root.controller.updateTheme({backgroundColor:text,backgroundType:0}) }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        Button { text:qsTr("FUNDO COM IMAGEM"); Layout.fillWidth:true; onClicked:themeBackgroundDialog.open() }
                        Button { text:qsTr("FUNDO SÓLIDO"); onClicked:root.controller.updateTheme({backgroundType:0}) }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        ComboBox { Layout.fillWidth:true; model:["left","center","right"]; currentIndex:Math.max(0,model.indexOf(root.controller.activeTheme.horizontalAlignment)); onActivated:root.controller.updateTheme({horizontalAlignment:currentText}) }
                        ComboBox { Layout.fillWidth:true; model:["top","center","bottom"]; currentIndex:Math.max(0,model.indexOf(root.controller.activeTheme.verticalAlignment)); onActivated:root.controller.updateTheme({verticalAlignment:currentText}) }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        CheckBox { text:qsTr("Contorno"); checked:root.controller.activeTheme.outline || false; onToggled:root.controller.updateTheme({outline:checked}) }
                        CheckBox { text:qsTr("Sombra"); checked:root.controller.activeTheme.shadow || false; onToggled:root.controller.updateTheme({shadow:checked}) }
                        ComboBox { Layout.fillWidth:true; model:["fade","none"]; currentIndex:Math.max(0,model.indexOf(root.controller.activeTheme.transition)); onActivated:root.controller.updateTheme({transition:currentText}) }
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { text:qsTr("MÚSICAS"); color:"#8da0bc"; font.bold:true; font.pixelSize:11 }
                    TextField { Layout.fillWidth:true; placeholderText:qsTr("Buscar por título, autor ou letra"); text:root.controller.songSearch; onTextEdited:root.controller.songSearch=text }
                    RowLayout {
                        Layout.fillWidth:true
                        ComboBox { id:songPicker; Layout.fillWidth:true; model:root.controller.songs; textRole:"title"; valueRole:"id"; onActivated:root.controller.selectSong(currentValue) }
                        Button { text:qsTr("ABRIR"); enabled:songPicker.currentValue!==undefined; onClicked:root.controller.selectSong(songPicker.currentValue) }
                        Button { text:qsTr("EXCLUIR"); enabled:songPicker.currentValue!==undefined; onClicked:root.controller.deleteTextPresentation(songPicker.currentValue) }
                    }
                    TextField { id:newSongTitle; Layout.fillWidth:true; placeholderText:qsTr("Título da música") }
                    TextField { id:newSongAuthor; Layout.fillWidth:true; placeholderText:qsTr("Autor") }
                    TextArea {
                        id:newSongLyrics
                        Layout.fillWidth:true; Layout.preferredHeight:150; wrapMode:TextEdit.Wrap
                        placeholderText:qsTr("V1\nLetra do verso\n\nC\nLetra do coro\n\nV2\nPróximo verso")
                    }
                    TextField { id:newSongSequence; Layout.fillWidth:true; placeholderText:qsTr("Sequência: V1 C V2 C P C") }
                    Button {
                        text:qsTr("CRIAR MÚSICA"); Layout.fillWidth:true; highlighted:true
                        onClicked:{
                            if(newSongTitle.text.trim().length>0&&newSongLyrics.text.trim().length>0){
                                root.controller.createSong(newSongTitle.text,newSongAuthor.text,newSongLyrics.text,newSongSequence.text)
                                newSongTitle.clear();newSongAuthor.clear();newSongLyrics.clear();newSongSequence.clear()
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        TextField { id:editSongSequence; Layout.fillWidth:true; placeholderText:qsTr("Sequência atual"); text:root.controller.songSequence }
                        Button { text:qsTr("SALVAR SEQUÊNCIA"); enabled:root.controller.currentPresentationId.length>0; onClicked:root.controller.updateSongSequence(editSongSequence.text) }
                    }

                    EventsArea {
                        Layout.fillWidth: true
                        context: root.controller.eventContext
                        sourceController: root.controller
                        onClearHistoryRequested: clearHistoryDialog.open()
                    }

                    MaintenanceArea {
                        Layout.fillWidth: true
                        context: root.controller.maintenanceContext
                        onRestoreRequested: restoreDialog.open()
                        onDiagnosticsExportRequested: diagnosticExportDialog.open()
                    }

                    Rectangle { visible:root.controller.debugEnabled;Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { visible:root.controller.debugEnabled;text: qsTr("DESENVOLVIMENTO"); color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
                    Label {
                        visible: root.controller.debugEnabled && root.controller.debugSimulatedOutputs
                        text: qsTr("Saídas simuladas: %1").arg(root.controller.simulatedOutputCount)
                        color: "#c8d5e8"
                        font.pixelSize: 12
                    }
                    Slider {
                        visible: root.controller.debugEnabled && root.controller.debugSimulatedOutputs
                        Layout.fillWidth: true
                        from: 1; to: 5; stepSize: 1
                        value: root.controller.simulatedOutputCount
                        onMoved: root.controller.simulatedOutputCount = Math.round(value)
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            text: root.controller.undoLabel.length > 0
                                  ? qsTr("DESFAZER: %1").arg(root.controller.undoLabel.toUpperCase())
                                  : qsTr("DESFAZER")
                            Layout.fillWidth: true
                            enabled: root.controller.canUndo
                            onClicked: root.controller.undo()
                        }
                        Button {
                            text: root.controller.redoLabel.length > 0
                                  ? qsTr("REFAZER: %1").arg(root.controller.redoLabel.toUpperCase())
                                  : qsTr("REFAZER")
                            Layout.fillWidth: true
                            enabled: root.controller.canRedo
                            onClicked: root.controller.redo()
                        }
                    }
                    Button {
                        text: root.controller.outputContext.blackout ? qsTr("RESTAURAR APRESENTAÇÃO") : qsTr("BLACKOUT")
                        Layout.fillWidth: true
                        highlighted: true
                        onClicked: root.controller.outputContext.blackout = !root.controller.outputContext.blackout
                    }
                }
            }
        }
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
    Shortcut { sequence: "Space"; enabled: root.controller.textVisible && !slideTextEditor.activeFocus; onActivated: root.controller.nextTextSlide() }
    Shortcut { sequence: "Home"; enabled: root.controller.textVisible; onActivated: root.controller.firstTextSlide() }
    Shortcut { sequence: "End"; enabled: root.controller.textVisible; onActivated: root.controller.lastTextSlide() }
    Shortcut { sequence: root.controller.shortcuts.stop; enabled: root.controller.textVisible; onActivated: root.controller.stopTextPresentation() }
    Shortcut { sequence: StandardKey.Undo; enabled: root.controller.canUndo; onActivated: root.controller.undo() }
    Shortcut { sequence: StandardKey.Redo; enabled: root.controller.canRedo; onActivated: root.controller.redo() }
    Shortcut { sequence: root.controller.shortcuts.blackout; onActivated: root.controller.outputContext.blackout = !root.controller.outputContext.blackout }
    Shortcut { sequence: root.controller.shortcuts.quickBible; onActivated: quickBibleSearch.openWithText("") }
    Shortcut { sequence: "Ctrl+Shift+B"; onActivated: root.controller.maintenanceContext.createBackup() }
    Shortcut { sequence: "Ctrl+S"; enabled: root.controller.currentSlideId.length > 0; onActivated: root.controller.updateTextSlide(root.controller.currentSlideId, slideLabelEditor.text, slideTextEditor.text) }
    Shortcut { sequence: "F5"; onActivated: root.controller.maintenanceContext.checkForUpdates() }
    Shortcut { sequence: "Ctrl+Shift+O"; onActivated: { root.show(); root.raise(); root.requestActivate() } }
}
