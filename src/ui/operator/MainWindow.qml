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
    title: "HolyScreen — Operação"
    width: 1360
    height: 820
    minimumWidth: 1100
    minimumHeight: 680
    property url pendingRestoreSource
    readonly property var optionalBibleTranslations: [{"id":"", "displayName":"Nenhuma"}]
                                                     .concat(root.controller.bibleTranslations)
    readonly property var outputBibleTranslations: [{"id":"", "displayName":"Bíblia: composição padrão"}]
                                                   .concat(root.controller.bibleTranslations)
    readonly property var outputRoles: [{"id":"audience", "displayName":"Saída: público"},
                                        {"id":"stage", "displayName":"Saída: palco"},
                                        {"id":"broadcast", "displayName":"Saída: transmissão"}]
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
        title: "Comunicação com o palco"
        modal: true
        width: 560
        standardButtons: Dialog.Close
        onOpened: stageMessageEditor.text = root.controller.stageMessage
        contentItem: ColumnLayout {
            spacing: 12
            Label {
                text: "A mensagem aparece somente nas telas configuradas como palco."
                color: "#8da0bc"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            TextArea {
                id: stageMessageEditor
                Layout.fillWidth: true
                Layout.preferredHeight: 110
                placeholderText: "Ex.: Pastor, faltam 5 minutos"
                wrapMode: TextEdit.Wrap
            }
            RowLayout {
                Layout.fillWidth: true
                Button {
                    text: "ENVIAR AO PALCO"
                    enabled: stageMessageEditor.text.trim().length > 0
                    onClicked: root.controller.stageMessage = stageMessageEditor.text
                }
                Button {
                    text: "LIMPAR"
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
        title: "Comunicação ao vivo"
        modal: true
        width: 680
        height: Math.min(root.height - 80, 650)
        standardButtons: Dialog.Close
        contentItem: ScrollView {
            clip: true
            ColumnLayout {
                width: parent.width
                spacing: 14
                Label { text: "MENSAGEM NO TOPO"; color: "#8da0bc"; font.bold: true }
                TextField { id: audienceMessageEditor; Layout.fillWidth: true; placeholderText: "Mensagem para o público" }
                RowLayout {
                    Button { text: "EXIBIR"; onClicked: root.controller.setAudienceMessage(audienceMessageEditor.text) }
                    Button { text: "LIMPAR"; onClicked: root.controller.setAudienceMessage("") }
                }
                Label { text: "ALERTA CENTRAL"; color: "#8da0bc"; font.bold: true }
                TextField { id: alertEditor; Layout.fillWidth: true; placeholderText: "Aviso importante" }
                RowLayout {
                    Button { text: "EXIBIR ALERTA"; onClicked: root.controller.setAlertMessage(alertEditor.text) }
                    Button { text: "LIMPAR"; onClicked: root.controller.setAlertMessage("") }
                }
                Label { text: "LOWER THIRD"; color: "#8da0bc"; font.bold: true }
                TextField { id: lowerThirdTitleEditor; Layout.fillWidth: true; placeholderText: "Nome / título" }
                TextField { id: lowerThirdSubtitleEditor; Layout.fillWidth: true; placeholderText: "Descrição / igreja" }
                RowLayout {
                    Button { text: "EXIBIR LOWER THIRD"; onClicked: root.controller.setLowerThird(lowerThirdTitleEditor.text, lowerThirdSubtitleEditor.text) }
                    Button { text: "LIMPAR"; onClicked: root.controller.setLowerThird("", "") }
                }
                Label { text: "CONTAGEM REGRESSIVA"; color: "#8da0bc"; font.bold: true }
                RowLayout {
                    Label { text: "Minutos" }
                    SpinBox { id: countdownMinutes; from: 0; to: 999; value: 5 }
                    Label { text: "Segundos" }
                    SpinBox { id: countdownSeconds; from: 0; to: 59; value: 0 }
                    Button {
                        text: root.controller.countdownRunning ? root.controller.countdownText : "INICIAR"
                        onClicked: root.controller.startCountdown(countdownMinutes.value * 60 + countdownSeconds.value)
                    }
                    Button { text: "PARAR"; onClicked: root.controller.stopCountdown() }
                }
                Label { text: "CRONÔMETRO"; color: "#8da0bc"; font.bold: true }
                RowLayout {
                    Label { text: root.controller.stopwatchText; font.pixelSize: 22; font.bold: true }
                    Button {
                        text: root.controller.stopwatchRunning ? "PAUSAR" : "INICIAR"
                        onClicked: root.controller.stopwatchRunning
                                   ? root.controller.pauseStopwatch()
                                   : root.controller.startStopwatch()
                    }
                    Button { text: "ZERAR"; onClicked: root.controller.resetStopwatch() }
                }
            }
        }
    }

    FileDialog {
        id: wallpaperDialog
        title: "Selecionar wallpaper"
        nameFilters: ["Imagens (*.jpg *.jpeg *.png *.webp)"]
        onAccepted: root.controller.wallpaperSource = selectedFile
    }
    Dialog {
        id: clearHistoryDialog
        title: "Limpar histórico?"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: root.controller.eventContext.clearHistory()
        Label { text:"Essa ação remove definitivamente os registros de execução.";wrapMode:Text.WordWrap }
    }
    FileDialog {
        id: restoreDialog
        title:"Selecionar backup do HolyScreen"
        nameFilters:["Banco HolyScreen (*.db)"]
        onAccepted:{root.pendingRestoreSource=selectedFile;restoreConfirmDialog.open()}
    }
    FileDialog {
        id: diagnosticExportDialog
        title: "Exportar diagnóstico do HolyScreen"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Arquivo ZIP (*.zip)"]
        defaultSuffix: "zip"
        onAccepted: root.controller.maintenanceContext.exportDiagnostics(selectedFile)
    }
    Dialog {
        id:restoreConfirmDialog;title:"Agendar restauração?";modal:true
        standardButtons:Dialog.Ok|Dialog.Cancel
        onAccepted:root.controller.maintenanceContext.scheduleRestore(root.pendingRestoreSource)
        Label {text:"O banco atual será preservado em um backup de segurança. A restauração será aplicada somente após reiniciar o app.";wrapMode:Text.WordWrap;width:420}
    }

    FileDialog {
        id: audioDialog
        title: "Importar áudios"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Áudios (*.mp3 *.wav *.flac *.m4a *.aac *.ogg *.opus *.wma *.aiff *.aif)"]
        onAccepted: root.controller.importAudioFiles(selectedFiles)
    }

    FileDialog {
        id: videoDialog
        title: "Importar vídeos"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Vídeos (*.mp4 *.mov *.m4v *.mkv *.webm *.avi *.wmv *.mpeg *.mpg)"]
        onAccepted: root.controller.importVideoFiles(selectedFiles)
    }

    FileDialog {
        id: imageDialog
        title: "Importar imagens"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Imagens (*.jpg *.jpeg *.png *.webp *.bmp *.gif *.tif *.tiff *.heic)"]
        onAccepted: root.controller.importImageFiles(selectedFiles)
    }
    FileDialog {
        id: bibleImportDialog
        title: "Importar JSON HolyScreen legado"
        nameFilters: ["HolyScreen Bíblia JSON (*.json)"]
        onAccepted: root.controller.importBibleTranslation(selectedFile)
    }
    FolderDialog {
        id: bibleFolderDialog
        title: "Selecionar repositório, data/canonical ou pasta da tradução"
        onAccepted: root.controller.importBibleFolder(selectedFolder)
    }
    Dialog {
        id: bibleOnlineImportDialog
        title: "Importar Bíblia de origem pública"
        modal: true
        width: 620
        standardButtons: Dialog.Close
        contentItem: ColumnLayout {
            spacing: 12
            Label {
                Layout.fillWidth: true
                text: "Use uma URL HTTPS pública. O repositório Git é clonado internamente; o ZIP é validado e extraído em staging temporário."
                wrapMode: Text.WordWrap
                color: "#b8c6dc"
            }
            Label { text: "REPOSITÓRIO GIT HTTPS"; color: "#8da0bc"; font.bold: true }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: bibleGitUrl
                    Layout.fillWidth: true
                    placeholderText: "https://github.com/usuario/repositorio.git"
                }
                Button {
                    text: "IMPORTAR GIT"
                    enabled: !root.controller.bibleImportRunning && bibleGitUrl.text.trim().length > 0
                    onClicked: {
                        if (root.controller.importBibleGit(bibleGitUrl.text))
                            bibleOnlineImportDialog.close()
                    }
                }
            }
            Label { text: "ARQUIVO ZIP HTTPS"; color: "#8da0bc"; font.bold: true }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: bibleZipUrl
                    Layout.fillWidth: true
                    placeholderText: "https://exemplo.org/biblias.zip"
                }
                Button {
                    text: "IMPORTAR ZIP"
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
        title: "Confirmar licenças das traduções"
        modal: true
        width: 580
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: root.controller.confirmBibleImportLicenses()
        contentItem: ColumnLayout {
            spacing: 10
            Label {
                Layout.fillWidth: true
                text: "As traduções abaixo não estão marcadas como domínio público. O HolyScreen não redistribui esse conteúdo. Confirme apenas se você tem permissão para importá-lo:"
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
        title: "Bíblia"
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
                Label { text: "TRADUÇÕES (ATÉ 3 SIMULTÂNEAS)"; color: "#8da0bc"; font.bold: true }
                Item { Layout.fillWidth: true }
                Button {
                    text: "IMPORTAR PASTA"
                    enabled: !root.controller.bibleImportRunning
                    onClicked: bibleFolderDialog.open()
                }
                Button {
                    text: "GIT / ZIP"
                    enabled: !root.controller.bibleImportRunning
                    onClicked: bibleOnlineImportDialog.open()
                }
                Button {
                    text: "JSON LEGADO"
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
                        text: "CANCELAR"
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
                Label { text: "PRINCIPAL"; color: "#8da0bc" }
                Label { text: "SECUNDÁRIA"; color: "#8da0bc" }
                Label { text: "TERCEIRA"; color: "#8da0bc" }
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
                Label { text: "ORIGEM:"; color: "#8da0bc"; font.bold: true }
                ComboBox {
                    id: bibleManagedTranslation
                    Layout.fillWidth: true
                    model: root.controller.bibleTranslations
                    textRole: "displayName"
                    valueRole: "id"
                }
                Label {
                    text: bibleManagedTranslation.currentIndex >= 0
                          ? (bibleManagedTranslation.model[bibleManagedTranslation.currentIndex].license || "origem legada")
                          : ""
                    color: "#8da0bc"
                }
                Button {
                    text: "ATUALIZAR DA ORIGEM"
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
                                     ? " • revisão " + item.sourceRevision.substring(0, 12) : ""
                    const publisher = item.publisher ? " • " + item.publisher : ""
                    return "Origem: " + item.sourceLocation + revision + publisher
                }
                color: "#64748b"
                elide: Text.ElideMiddle
                font.pixelSize: 11
            }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    Layout.fillWidth: true
                    placeholderText: "João 3:16, Jo 3 16 ou João 3.16"
                    text: root.controller.bibleReferenceInput
                    onTextEdited: root.controller.bibleReferenceInput = text
                    onAccepted: root.controller.searchBibleReference()
                }
                Button { text: "BUSCAR"; highlighted: true; onClicked: root.controller.searchBibleReference() }
            }
            Label {
                visible: root.controller.bibleTranslations.length === 0
                Layout.fillWidth: true
                text: "Importe uma pasta/repositório canônico, Git HTTPS, ZIP público ou JSON legado. Os textos bíblicos não são embutidos por questões de licenciamento."
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
                        Button { text: "APRESENTAR"; onClicked: root.controller.showBibleVerse(bibleResultDelegate.index) }
                    }
                }
                Label {
                    anchors.centerIn: parent
                    visible: bibleResultsList.count === 0 && root.controller.bibleTranslations.length > 0
                    text: "Digite uma referência para localizar os versículos"
                    color: "#64748b"
                }
            }
        }
    }
    FolderDialog {
        id: mediaFolderDialog
        title: "Adicionar pasta de mídia"
        onAccepted: root.controller.addMediaFolder(selectedFolder)
    }
    Dialog {
        id: mediaLibraryDialog
        title: "Biblioteca de pastas"
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
                Label { text: "PASTAS SELECIONADAS"; color: "#8da0bc"; font.bold: true }
                Item { Layout.fillWidth: true }
                Button { text: "+ PASTA"; onClicked: mediaFolderDialog.open() }
                Button { text: "ATUALIZAR"; onClicked: root.controller.rescanMediaFolders() }
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
                        ToolButton { text: "×"; onClicked: root.controller.removeMediaFolder(mediaFolderDelegate.modelData.path) }
                    }
                }
                Label {
                    anchors.centerIn: parent
                    visible: mediaFoldersList.count === 0
                    text: "Adicione uma ou mais pastas de áudio, vídeo ou imagens"
                    color: "#64748b"
                }
            }
            TabBar {
                id: mediaTypeTabs
                Layout.fillWidth: true
                TabButton { text: "ÁUDIOS (" + root.controller.folderAudioFiles.length + ")" }
                TabButton { text: "VÍDEOS (" + root.controller.folderVideoFiles.length + ")" }
                TabButton { text: "IMAGENS (" + root.controller.folderImageFiles.length + ")" }
            }
            TextField {
                Layout.fillWidth: true
                placeholderText: "Pesquisar por nome de arquivo"
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
                            text: folderMediaDelegate.modelData.inPlaylist ? "NA PLAYLIST" : "+ PLAYLIST"
                            enabled: !folderMediaDelegate.modelData.inPlaylist
                            onClicked: root.controller.addCatalogFileToPlaylist(folderMediaDelegate.modelData.path)
                        }
                    }
                }
                Label {
                    anchors.centerIn: parent
                    visible: folderMediaList.count === 0
                    text: root.controller.mediaFolders.length === 0
                          ? "Nenhuma pasta selecionada"
                          : "Nenhum arquivo encontrado para esta pesquisa"
                    color: "#64748b"
                }
            }
        }
    }
    FileDialog {
        id: themeBackgroundDialog
        title: "Selecionar fundo do tema"
        nameFilters: ["Imagens (*.jpg *.jpeg *.png *.webp *.bmp)"]
        onAccepted: root.controller.updateTheme({backgroundType: 2, backgroundImage: selectedFile.toString()})
    }

    menuBar: MenuBar {
        visible: root.controller.debugEnabled
        Menu {
            title: "Debug"
            MenuItem {
                text: "Ativar modo de debug"
                checkable: true
                checked: root.controller.debugEnabled
                onTriggered: root.controller.debugEnabled = checked
            }
            MenuSeparator { }
            MenuItem {
                text: "Múltiplas saídas simuladas"
                checkable: true
                enabled: root.controller.debugEnabled
                checked: root.controller.debugSimulatedOutputs
                onTriggered: root.controller.debugSimulatedOutputs = checked
            }
            MenuItem {
                text: "Diagnósticos e benchmark"
                checkable: true
                enabled: root.controller.debugEnabled
                checked: root.controller.debugDiagnostics
                onTriggered: root.controller.debugDiagnostics = checked
            }
            MenuItem {
                text: "Registrar mensagens DEBUG"
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
                    text: "HolyScreen"
                    color: "#f2f4f5"
                    font.bold: true
                    font.pixelSize: 15
                }
                ToolButton { text: "Live"; onClicked: liveDialog.open() }
                ToolButton { text: "Prévia" }
                ToolButton { text: "Agenda" }
                ToolButton { text: "Biblioteca"; font.bold: true; onClicked: mediaLibraryDialog.open() }
                ToolButton { text: "Integrações"; onClicked: integrationsArea.open() }
                ToolButton {
                    text: "Automações"
                    onClicked: automationsArea.open()
                    ToolTip.visible: hovered && !root.controller.automationsEnabled
                    ToolTip.text: "Automações pausadas"
                }
                Item { Layout.fillWidth: true }
                ToolButton {
                    text: "⚙"
                    Accessible.name: "Configurações"
                    onClicked: settingsDialog.open()
                }
                ToolButton { text: "⛶"; onClicked: root.visibility = root.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
                Button { text: root.controller.outputContext.blackout ? "Restaurar" : "Ao vivo"; highlighted: true; onClicked: root.controller.outputContext.blackout = false }
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
                        text: "★  FAVORITOS"
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
                                  + (modelData.fileName || modelData.title || "Sem título")
                            font.pixelSize: 11
                            onClicked: root.playFavorite(modelData.path)
                            ToolTip.visible: hovered
                            ToolTip.text: "Reproduzir " + (modelData.fileName || modelData.title)
                        }
                        Label {
                            anchors.centerIn: parent
                            visible: favoriteMediaList.count === 0
                            text: "Clique com o botão direito em uma mídia para adicioná-la"
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
                Label { text: "TELAS"; color: "#e6edf7"; font.bold: true; font.pixelSize: 15 }
                Label {
                    text: "Selecione até cinco saídas e defina público ou palco."
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
                                            text: screenDelegate.modelData.primary ? "OPERADOR • tela principal"
                                                  : !screenDelegate.modelData.selected ? "DISPONÍVEL"
                                                  : screenDelegate.modelData.role === "stage" ? "PALCO"
                                                  : screenDelegate.modelData.role === "broadcast" ? "TRANSMISSÃO"
                                                  : screenDelegate.modelData.role === "confidence" ? "CONFERÊNCIA"
                                                  : screenDelegate.modelData.role === "custom" ? "PERSONALIZADA"
                                                  : "PÚBLICO"
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
                                            text: "Exibir mídia nesta tela"
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
                    text: "ATIVAR TODAS"
                    Layout.fillWidth: true
                    onClicked: root.controller.enableAllScreens()
                }
                Button {
                    text: "IDENTIFICAR TELAS"
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
                    Label { text: "PRÉVIA DE SAÍDA"; color: "#e6edf7"; font.bold: true; font.pixelSize: 15 }
                    Item { Layout.fillWidth: true }
                    Label { text: "Wallpaper + Relógio"; color: "#8da0bc"; font.pixelSize: 12 }
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
                            outputLabel: root.previewOutputCount > 1 ? "SIMULAÇÃO " + (index + 1) : "PRÉVIA"
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
                                Label { text: "PLAYLIST MULTIMÍDIA"; color: "#e6edf7"; font.bold: true; font.pixelSize: 12 }
                                Item { Layout.fillWidth: true }
                                Button { text: "PASTAS"; onClicked: mediaLibraryDialog.open() }
                                Button { text: "+ ÁUDIO"; onClicked: audioDialog.open() }
                                Button { text: "+ VÍDEO"; onClicked: videoDialog.open() }
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
                                        ToolButton { text: "↑"; enabled: mediaDelegate.index > 0; onClicked: root.controller.moveMedia(mediaDelegate.modelData.id, mediaDelegate.index - 1) }
                                        ToolButton { text: "↓"; enabled: mediaDelegate.index + 1 < mediaList.count; onClicked: root.controller.moveMedia(mediaDelegate.modelData.id, mediaDelegate.index + 1) }
                                        ToolButton {
                                            text: "×"
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
                                    text: "Importe ou arraste áudios e vídeos"
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
                                      ? root.controller.currentMediaTitle : "Nenhuma mídia selecionada"
                                color: "#eff6ff"
                                font.bold: true
                                font.pixelSize: 14
                                elide: Text.ElideRight
                            }
                            Label {
                                text: (root.controller.currentMediaType.length > 0
                                       ? root.controller.currentMediaType.toUpperCase() + " · " : "")
                                      + root.controller.mediaState.toUpperCase()
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
                                Button { text: "ANTERIOR"; onClicked: root.controller.mediaContext.previousMedia() }
                                Button {
                                    text: root.controller.mediaState === "playing" ? "PAUSAR" : "TOCAR"
                                    highlighted: true
                                    onClicked: {
                                        if (root.controller.currentMediaId.length === 0 && mediaList.count > 0)
                                            root.controller.playMedia(root.controller.mediaPlaylist[0].id)
                                        else
                                            root.controller.mediaContext.toggleMediaPause()
                                    }
                                }
                                Button { text: "PARAR"; onClicked: root.controller.mediaContext.stopMedia() }
                                Button { text: "PRÓXIMO"; onClicked: root.controller.mediaContext.nextMedia() }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: "VOLUME"; color: "#8da0bc"; font.pixelSize: 10 }
                                Slider {
                                    Layout.fillWidth: true
                                    from: 0; to: 1; stepSize: 0.01
                                    value: root.controller.mediaVolume
                                    onMoved: root.controller.mediaVolume = value
                                }
                                ComboBox {
                                    model: ["Não repetir", "Repetir uma", "Repetir playlist"]
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
                    Label { text: "CONTROLES"; color: "#e6edf7"; font.bold: true; font.pixelSize: 15 }
                    Label { text: "WALLPAPER"; color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
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
                        Button { text: "ESCOLHER IMAGEM"; Layout.fillWidth: true; onClicked: wallpaperDialog.open() }
                        Button { text: "LIMPAR"; onClicked: root.controller.wallpaperSource = "" }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["cover", "contain", "stretch", "center"]
                        currentIndex: Math.max(0, model.indexOf(root.controller.wallpaperFit))
                        onActivated: root.controller.wallpaperFit = currentText
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { text: "RELÓGIO"; color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
                    CheckBox {
                        text: "Exibir relógio"
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
                        placeholderText: "Fonte"
                        text: root.controller.clockFontFamily
                        onEditingFinished: root.controller.clockFontFamily = text
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "Tamanho"; color: "#c8d5e8" }
                        SpinBox {
                            from: 16; to: 240
                            value: root.controller.clockFontSize
                            onValueModified: root.controller.clockFontSize = value
                        }
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { text: "IMAGENS"; color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: "IMPORTAR"; Layout.fillWidth: true; onClicked: imageDialog.open() }
                        Button {
                            text: "REMOVER"
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
                                     : (currentText.length > 0 ? currentText : "Nenhuma imagem")
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        ToolButton {
                            text: "↑"
                            enabled: imagePicker.currentIndex > 0
                            onClicked: root.controller.moveImage(imagePicker.currentValue, imagePicker.currentIndex - 1)
                        }
                        ToolButton {
                            text: "↓"
                            enabled: imagePicker.currentIndex >= 0 && imagePicker.currentIndex + 1 < imagePicker.count
                            onClicked: root.controller.moveImage(imagePicker.currentValue, imagePicker.currentIndex + 1)
                        }
                        Button {
                            text: "EXIBIR"
                            Layout.fillWidth: true
                            highlighted: true
                            enabled: imagePicker.currentValue !== undefined && imagePicker.currentValue.length > 0
                            onClicked: root.controller.showImage(imagePicker.currentValue)
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: "ANTERIOR"; Layout.fillWidth: true; onClicked: root.controller.previousImage() }
                        Button { text: "PRÓXIMA"; Layout.fillWidth: true; onClicked: root.controller.nextImage() }
                        Button { text: "PARAR"; onClicked: root.controller.stopImage() }
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
                            text: "Autoplay"
                            checked: root.controller.imageAutoplay
                            onToggled: root.controller.imageAutoplay = checked
                        }
                        Label { text: "segundos"; color: "#8da0bc"; font.pixelSize: 10 }
                        SpinBox {
                            from: 1; to: 3600
                            value: Math.round(root.controller.imageIntervalMs / 1000)
                            onValueModified: root.controller.imageIntervalMs = value * 1000
                        }
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { text: "APRESENTAÇÕES DE TEXTO"; color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField { id: newPresentationTitle; Layout.fillWidth: true; placeholderText: "Título da apresentação" }
                        Button {
                            text: "NOVA"
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
                            text: "EXCLUIR"
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
                        placeholderText: "Rótulo do slide"
                        text: root.controller.currentSlideLabel
                    }
                    TextArea {
                        id: slideTextEditor
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        placeholderText: "Texto do slide"
                        text: root.controller.currentSlideText
                        wrapMode: TextEdit.Wrap
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            text: "SALVAR"
                            Layout.fillWidth: true
                            enabled: root.controller.currentSlideId.length > 0
                            onClicked: root.controller.updateTextSlide(root.controller.currentSlideId, slideLabelEditor.text, slideTextEditor.text)
                        }
                        Button {
                            text: "+ SLIDE"
                            enabled: root.controller.currentPresentationId.length > 0
                            onClicked: root.controller.addTextSlide(String(root.controller.textSlides.length + 1), "Novo slide")
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: "DUPLICAR"; enabled: root.controller.currentSlideId.length > 0; onClicked: root.controller.duplicateTextSlide(root.controller.currentSlideId) }
                        Button { text: "DIVIDIR"; enabled: root.controller.currentSlideId.length > 0; onClicked: root.controller.splitTextSlide(root.controller.currentSlideId, slideTextEditor.cursorPosition) }
                        Button { text: "EXCLUIR"; enabled: root.controller.currentSlideId.length > 0; onClicked: root.controller.removeTextSlide(root.controller.currentSlideId) }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        ToolButton { text: "↑"; enabled: slidePicker.currentIndex > 0; onClicked: root.controller.moveTextSlide(root.controller.currentSlideId, slidePicker.currentIndex - 1) }
                        ToolButton { text: "↓"; enabled: slidePicker.currentIndex + 1 < slidePicker.count; onClicked: root.controller.moveTextSlide(root.controller.currentSlideId, slidePicker.currentIndex + 1) }
                        Button { text: "ANTERIOR"; Layout.fillWidth: true; onClicked: root.controller.previousTextSlide() }
                        Button { text: "EXIBIR"; highlighted: true; enabled: slidePicker.currentIndex >= 0; onClicked: root.controller.showTextSlide(slidePicker.currentIndex) }
                        Button { text: "PRÓXIMO"; Layout.fillWidth: true; onClicked: root.controller.nextTextSlide() }
                    }
                    Button { text: "VOLTAR AO WALLPAPER"; Layout.fillWidth: true; onClicked: root.controller.stopTextPresentation() }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { text: "TEMAS"; color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField { id: newThemeName; Layout.fillWidth: true; placeholderText: "Nome do tema" }
                        Button { text: "NOVO"; onClicked: { if(newThemeName.text.trim().length>0){root.controller.createTheme(newThemeName.text);newThemeName.clear()} } }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        ComboBox { id: themePicker; Layout.fillWidth: true; model: root.controller.themes; textRole:"name"; valueRole:"id"; onActivated: root.controller.applyTheme(currentValue) }
                        Button { text:"APLICAR"; enabled:themePicker.currentValue!==undefined; onClicked:root.controller.applyTheme(themePicker.currentValue) }
                        Button { text:"EXCLUIR"; enabled:root.controller.themes.length>1; onClicked:root.controller.deleteTheme(themePicker.currentValue) }
                    }
                    TextField { Layout.fillWidth:true; placeholderText:"Família da fonte"; text:root.controller.activeTheme.fontFamily || ""; onEditingFinished:root.controller.updateTheme({fontFamily:text}) }
                    RowLayout {
                        Layout.fillWidth:true
                        Label { text:"Fonte"; color:"#8da0bc" }
                        SpinBox { from:28;to:240;value:root.controller.activeTheme.fontSize || 72; onValueModified:root.controller.updateTheme({fontSize:value}) }
                        Label { text:"Margem"; color:"#8da0bc" }
                        SpinBox { from:0;to:400;value:root.controller.activeTheme.margin || 64; onValueModified:root.controller.updateTheme({margin:value}) }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        TextField { Layout.fillWidth:true; placeholderText:"Cor do texto"; text:root.controller.activeTheme.textColor || "#ffffff"; onEditingFinished:root.controller.updateTheme({textColor:text}) }
                        TextField { Layout.fillWidth:true; placeholderText:"Cor de fundo"; text:root.controller.activeTheme.backgroundColor || "#000000"; onEditingFinished:root.controller.updateTheme({backgroundColor:text,backgroundType:0}) }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        Button { text:"FUNDO COM IMAGEM"; Layout.fillWidth:true; onClicked:themeBackgroundDialog.open() }
                        Button { text:"FUNDO SÓLIDO"; onClicked:root.controller.updateTheme({backgroundType:0}) }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        ComboBox { Layout.fillWidth:true; model:["left","center","right"]; currentIndex:Math.max(0,model.indexOf(root.controller.activeTheme.horizontalAlignment)); onActivated:root.controller.updateTheme({horizontalAlignment:currentText}) }
                        ComboBox { Layout.fillWidth:true; model:["top","center","bottom"]; currentIndex:Math.max(0,model.indexOf(root.controller.activeTheme.verticalAlignment)); onActivated:root.controller.updateTheme({verticalAlignment:currentText}) }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        CheckBox { text:"Contorno"; checked:root.controller.activeTheme.outline || false; onToggled:root.controller.updateTheme({outline:checked}) }
                        CheckBox { text:"Sombra"; checked:root.controller.activeTheme.shadow || false; onToggled:root.controller.updateTheme({shadow:checked}) }
                        ComboBox { Layout.fillWidth:true; model:["fade","none"]; currentIndex:Math.max(0,model.indexOf(root.controller.activeTheme.transition)); onActivated:root.controller.updateTheme({transition:currentText}) }
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { text:"MÚSICAS"; color:"#8da0bc"; font.bold:true; font.pixelSize:11 }
                    TextField { Layout.fillWidth:true; placeholderText:"Buscar por título, autor ou letra"; text:root.controller.songSearch; onTextEdited:root.controller.songSearch=text }
                    RowLayout {
                        Layout.fillWidth:true
                        ComboBox { id:songPicker; Layout.fillWidth:true; model:root.controller.songs; textRole:"title"; valueRole:"id"; onActivated:root.controller.selectSong(currentValue) }
                        Button { text:"ABRIR"; enabled:songPicker.currentValue!==undefined; onClicked:root.controller.selectSong(songPicker.currentValue) }
                        Button { text:"EXCLUIR"; enabled:songPicker.currentValue!==undefined; onClicked:root.controller.deleteTextPresentation(songPicker.currentValue) }
                    }
                    TextField { id:newSongTitle; Layout.fillWidth:true; placeholderText:"Título da música" }
                    TextField { id:newSongAuthor; Layout.fillWidth:true; placeholderText:"Autor" }
                    TextArea {
                        id:newSongLyrics
                        Layout.fillWidth:true; Layout.preferredHeight:150; wrapMode:TextEdit.Wrap
                        placeholderText:"V1\nLetra do verso\n\nC\nLetra do coro\n\nV2\nPróximo verso"
                    }
                    TextField { id:newSongSequence; Layout.fillWidth:true; placeholderText:"Sequência: V1 C V2 C P C" }
                    Button {
                        text:"CRIAR MÚSICA"; Layout.fillWidth:true; highlighted:true
                        onClicked:{
                            if(newSongTitle.text.trim().length>0&&newSongLyrics.text.trim().length>0){
                                root.controller.createSong(newSongTitle.text,newSongAuthor.text,newSongLyrics.text,newSongSequence.text)
                                newSongTitle.clear();newSongAuthor.clear();newSongLyrics.clear();newSongSequence.clear()
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        TextField { id:editSongSequence; Layout.fillWidth:true; placeholderText:"Sequência atual"; text:root.controller.songSequence }
                        Button { text:"SALVAR SEQUÊNCIA"; enabled:root.controller.currentPresentationId.length>0; onClicked:root.controller.updateSongSequence(editSongSequence.text) }
                    }

                    Rectangle { Layout.fillWidth:true; Layout.preferredHeight:1; color:"#24334b" }
                    Label { text:"PLAYLIST DE CULTO"; color:"#8da0bc"; font.bold:true; font.pixelSize:11 }
                    RowLayout {
                        Layout.fillWidth:true
                        TextField { id:newEventTitle; Layout.fillWidth:true; placeholderText:"Nome do culto" }
                        TextField { id:newEventDate; Layout.preferredWidth:120; placeholderText:"Data/hora" }
                        Button { text:"NOVO"; onClicked:{if(newEventTitle.text.trim().length>0){root.controller.eventContext.createEvent(newEventTitle.text,newEventDate.text);newEventTitle.clear();newEventDate.clear()}} }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        ComboBox { id:eventPicker; Layout.fillWidth:true; model:root.controller.eventContext.events; textRole:"title";valueRole:"id";onActivated:root.controller.eventContext.selectEvent(currentValue) }
                        Button { text:"ABRIR"; enabled:eventPicker.currentValue!==undefined;onClicked:root.controller.eventContext.selectEvent(eventPicker.currentValue) }
                        Button { text:"EXCLUIR";enabled:root.controller.eventContext.currentEventId.length>0;onClicked:root.controller.eventContext.deleteEvent(root.controller.eventContext.currentEventId) }
                    }
                    Label { text:"Duração total: "+root.formatDuration(root.controller.eventContext.eventDurationMs);color:"#c8d5e8" }
                    Flow {
                        Layout.fillWidth:true;spacing:5
                        Button { text:"+ APRESENTAÇÃO";enabled:root.controller.eventContext.currentEventId.length>0&&root.controller.currentPresentationId.length>0;onClicked:root.controller.eventContext.addEventItem(root.controller.currentPresentationType,root.controller.currentPresentationId,root.controller.currentPresentationTitle,0) }
                        Button { text:"+ IMAGEM";enabled:root.controller.eventContext.currentEventId.length>0&&root.controller.currentImageId.length>0;onClicked:root.controller.eventContext.addEventItem("image",root.controller.currentImageId,root.controller.currentImageTitle,0) }
                        Button { text:"+ VÍDEO";enabled:root.controller.eventContext.currentEventId.length>0&&root.controller.currentVideoId.length>0;onClicked:root.controller.eventContext.addEventItem("video",root.controller.currentVideoId,root.controller.currentVideoTitle,root.controller.videoDurationMs) }
                        Button { text:"+ ÁUDIO";enabled:root.controller.eventContext.currentEventId.length>0&&root.controller.currentAudioId.length>0;onClicked:root.controller.eventContext.addEventItem("audio",root.controller.currentAudioId,root.controller.currentAudioTitle,root.controller.audioDurationMs) }
                    }
                    ListView {
                        id:eventItemsList;Layout.fillWidth:true;Layout.preferredHeight:Math.min(260,Math.max(60,contentHeight));clip:true;spacing:4
                        model:root.controller.eventContext.eventItems
                        delegate:Rectangle {
                            id:eventItemDelegate;required property var modelData;required property int index
                            width:ListView.view.width;height:46;color:"#142137";radius:6
                            RowLayout { anchors.fill:parent;anchors.margins:5
                                Label { text:"⋮⋮";color:"#8da0bc"
                                    DragHandler { target:null;onActiveChanged:{if(!active){const targetIndex=Math.max(0,Math.min(eventItemsList.count-1,eventItemDelegate.index+Math.round(translation.y/eventItemDelegate.height)));if(targetIndex!==eventItemDelegate.index)root.controller.eventContext.moveEventItem(eventItemDelegate.modelData.id,targetIndex)}} }
                                }
                                Label { text:eventItemDelegate.modelData.type.toUpperCase();color:"#70e1a7";font.pixelSize:9 }
                                Label { Layout.fillWidth:true;text:eventItemDelegate.modelData.title;color:"#eff6ff";elide:Text.ElideRight }
                                Label { text:root.formatDuration(eventItemDelegate.modelData.durationMs);color:"#8da0bc";font.pixelSize:10 }
                                ToolButton { text:"↑";enabled:eventItemDelegate.index>0;onClicked:root.controller.eventContext.moveEventItem(eventItemDelegate.modelData.id,eventItemDelegate.index-1) }
                                ToolButton { text:"↓";enabled:eventItemDelegate.index+1<eventItemsList.count;onClicked:root.controller.eventContext.moveEventItem(eventItemDelegate.modelData.id,eventItemDelegate.index+1) }
                                Button { text:"EXECUTAR";onClicked:root.controller.eventContext.executeEventItem(eventItemDelegate.modelData.id) }
                                ToolButton { text:"×";onClicked:root.controller.eventContext.removeEventItem(eventItemDelegate.modelData.id) }
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth:true;Layout.preferredHeight:1;color:"#24334b" }
                    RowLayout {
                        Layout.fillWidth:true
                        Label { text:"HISTÓRICO";color:"#8da0bc";font.bold:true;font.pixelSize:11 }
                        Item { Layout.fillWidth:true }
                        Button { text:"LIMPAR";enabled:root.controller.eventContext.history.length>0;onClicked:clearHistoryDialog.open() }
                    }
                    Label {
                        Layout.fillWidth:true
                        text:"Execuções: "+(root.controller.eventContext.historyReport.totalExecutions||0)+"  •  Mais executado: "+(root.controller.eventContext.historyReport.mostExecutedTitle||"—")
                        color:"#c8d5e8";wrapMode:Text.WordWrap
                    }
                    ListView {
                        Layout.fillWidth:true;Layout.preferredHeight:Math.min(220,Math.max(50,contentHeight));clip:true;spacing:3
                        model:root.controller.eventContext.history
                        delegate:Rectangle {
                            id: historyDelegate
                            required property var modelData;width:ListView.view.width;height:40;color:"#142137";radius:5
                            RowLayout {anchors.fill:parent;anchors.margins:6
                                Label {text:historyDelegate.modelData.type.toUpperCase();color:"#70e1a7";font.pixelSize:9}
                                Label {Layout.fillWidth:true;text:historyDelegate.modelData.title;color:"#eff6ff";elide:Text.ElideRight}
                                Label {text:historyDelegate.modelData.executedAt.replace("T"," ").slice(0,16);color:"#8da0bc";font.pixelSize:9}
                            }
                        }
                    }

                    Rectangle {Layout.fillWidth:true;Layout.preferredHeight:1;color:"#24334b"}
                    Label {text:"MANUTENÇÃO E DIAGNÓSTICOS";color:"#8da0bc";font.bold:true;font.pixelSize:11}
                    Label {visible:root.controller.maintenanceContext.recoveredFromCrash;text:"Uma sessão anterior terminou inesperadamente. Um snapshot de recuperação foi criado.";color:"#ffba70";wrapMode:Text.WordWrap;Layout.fillWidth:true}
                    RowLayout {
                        Layout.fillWidth:true
                        Button {text:"CRIAR BACKUP";Layout.fillWidth:true;onClicked:root.controller.maintenanceContext.createBackup()}
                        Button {text:"RESTAURAR";Layout.fillWidth:true;onClicked:restoreDialog.open()}
                        Button {text:"EXPORTAR DIAGNÓSTICO";Layout.fillWidth:true;onClicked:diagnosticExportDialog.open()}
                        Button {visible:root.controller.maintenanceContext.debugEnabled && root.controller.maintenanceContext.debugDiagnostics;text:"BENCHMARK";Layout.fillWidth:true;onClicked:root.controller.maintenanceContext.runBenchmark()}
                    }
                    Label {visible:root.controller.maintenanceContext.debugEnabled && root.controller.maintenanceContext.debugDiagnostics;Layout.fillWidth:true;wrapMode:Text.WordWrap;color:"#c8d5e8";text:"Versão "+(root.controller.maintenanceContext.diagnostics.version||"—")+" • Qt "+(root.controller.maintenanceContext.diagnostics.qtVersion||"—")+" • "+(root.controller.maintenanceContext.diagnostics.platform||"—")+" • Telas "+(root.controller.maintenanceContext.diagnostics.detectedScreens||0)+" • Ops/s "+(root.controller.maintenanceContext.diagnostics.benchmarkOperationsPerSecond||"—")}
                    TextField {Layout.fillWidth:true;placeholderText:"URL HTTPS do manifesto de atualização";text:root.controller.maintenanceContext.updateEndpoint;onEditingFinished:root.controller.maintenanceContext.updateEndpoint=text}
                    RowLayout {Layout.fillWidth:true
                        Button {text:"VERIFICAR ATUALIZAÇÕES";onClicked:root.controller.maintenanceContext.checkForUpdates()}
                        Label {Layout.fillWidth:true;text:root.controller.maintenanceContext.updateStatus;color:"#8da0bc";wrapMode:Text.WordWrap}
                    }

                    Rectangle { visible:root.controller.debugEnabled;Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { visible:root.controller.debugEnabled;text: "DESENVOLVIMENTO"; color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
                    Label {
                        visible: root.controller.debugEnabled && root.controller.debugSimulatedOutputs
                        text: "Saídas simuladas: " + root.controller.simulatedOutputCount
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
                                  ? "DESFAZER: " + root.controller.undoLabel.toUpperCase()
                                  : "DESFAZER"
                            Layout.fillWidth: true
                            enabled: root.controller.canUndo
                            onClicked: root.controller.undo()
                        }
                        Button {
                            text: root.controller.redoLabel.length > 0
                                  ? "REFAZER: " + root.controller.redoLabel.toUpperCase()
                                  : "REFAZER"
                            Layout.fillWidth: true
                            enabled: root.controller.canRedo
                            onClicked: root.controller.redo()
                        }
                    }
                    Button {
                        text: root.controller.outputContext.blackout ? "RESTAURAR APRESENTAÇÃO" : "BLACKOUT"
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
