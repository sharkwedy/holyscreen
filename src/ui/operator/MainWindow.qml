// qmllint disable unqualified
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root
    visible: true
    onClosing: Qt.quit()
    color: "#0b1220"
    title: "HolyScreen — Operação"
    width: 1360
    height: 820
    minimumWidth: 1100
    minimumHeight: 680
    property url pendingRestoreSource
    readonly property var optionalBibleTranslations: [{"id":"", "displayName":"Nenhuma"}]
                                                     .concat(presentationController.bibleTranslations)
    readonly property var outputBibleTranslations: [{"id":"", "displayName":"Bíblia: composição padrão"}]
                                                   .concat(presentationController.bibleTranslations)
    readonly property var outputRoles: [{"id":"audience", "displayName":"Saída: público"},
                                        {"id":"stage", "displayName":"Saída: palco"}]
    readonly property int previewOutputCount: presentationController.debugEnabled
                                              && presentationController.debugSimulatedOutputs
                                              ? presentationController.simulatedOutputCount : 1

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
        model: presentationController.outputWindows
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
        }
    }

    Dialog {
        id: stageMessageDialog
        title: "Comunicação com o palco"
        modal: true
        width: 560
        standardButtons: Dialog.Close
        onOpened: stageMessageEditor.text = presentationController.stageMessage
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
                    onClicked: presentationController.stageMessage = stageMessageEditor.text
                }
                Button {
                    text: "LIMPAR"
                    enabled: presentationController.stageMessage.length > 0
                    onClicked: {
                        stageMessageEditor.text = ""
                        presentationController.stageMessage = ""
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }
    }

    function playFavorite(path) {
        const mediaId = presentationController.addCatalogFileToPlaylist(path)
        if (mediaId.length > 0)
            presentationController.playMedia(mediaId)
    }
    SettingsDialog {
        id: settingsDialog
        controller: presentationController
        onOpenLibrary: mediaLibraryDialog.open()
        onChooseBackground: wallpaperDialog.open()
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
                    Button { text: "EXIBIR"; onClicked: presentationController.setAudienceMessage(audienceMessageEditor.text) }
                    Button { text: "LIMPAR"; onClicked: presentationController.setAudienceMessage("") }
                }
                Label { text: "ALERTA CENTRAL"; color: "#8da0bc"; font.bold: true }
                TextField { id: alertEditor; Layout.fillWidth: true; placeholderText: "Aviso importante" }
                RowLayout {
                    Button { text: "EXIBIR ALERTA"; onClicked: presentationController.setAlertMessage(alertEditor.text) }
                    Button { text: "LIMPAR"; onClicked: presentationController.setAlertMessage("") }
                }
                Label { text: "LOWER THIRD"; color: "#8da0bc"; font.bold: true }
                TextField { id: lowerThirdTitleEditor; Layout.fillWidth: true; placeholderText: "Nome / título" }
                TextField { id: lowerThirdSubtitleEditor; Layout.fillWidth: true; placeholderText: "Descrição / igreja" }
                RowLayout {
                    Button { text: "EXIBIR LOWER THIRD"; onClicked: presentationController.setLowerThird(lowerThirdTitleEditor.text, lowerThirdSubtitleEditor.text) }
                    Button { text: "LIMPAR"; onClicked: presentationController.setLowerThird("", "") }
                }
                Label { text: "CONTAGEM REGRESSIVA"; color: "#8da0bc"; font.bold: true }
                RowLayout {
                    Label { text: "Minutos" }
                    SpinBox { id: countdownMinutes; from: 0; to: 999; value: 5 }
                    Label { text: "Segundos" }
                    SpinBox { id: countdownSeconds; from: 0; to: 59; value: 0 }
                    Button {
                        text: presentationController.countdownRunning ? presentationController.countdownText : "INICIAR"
                        onClicked: presentationController.startCountdown(countdownMinutes.value * 60 + countdownSeconds.value)
                    }
                    Button { text: "PARAR"; onClicked: presentationController.stopCountdown() }
                }
                Label { text: "CRONÔMETRO"; color: "#8da0bc"; font.bold: true }
                RowLayout {
                    Label { text: presentationController.stopwatchText; font.pixelSize: 22; font.bold: true }
                    Button {
                        text: presentationController.stopwatchRunning ? "PAUSAR" : "INICIAR"
                        onClicked: presentationController.stopwatchRunning
                                   ? presentationController.pauseStopwatch()
                                   : presentationController.startStopwatch()
                    }
                    Button { text: "ZERAR"; onClicked: presentationController.resetStopwatch() }
                }
            }
        }
    }

    FileDialog {
        id: wallpaperDialog
        title: "Selecionar wallpaper"
        nameFilters: ["Imagens (*.jpg *.jpeg *.png *.webp)"]
        onAccepted: presentationController.wallpaperSource = selectedFile
    }
    Dialog {
        id: clearHistoryDialog
        title: "Limpar histórico?"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: presentationController.clearHistory()
        Label { text:"Essa ação remove definitivamente os registros de execução.";wrapMode:Text.WordWrap }
    }
    FileDialog {
        id: restoreDialog
        title:"Selecionar backup do HolyScreen"
        nameFilters:["Banco HolyScreen (*.db)"]
        onAccepted:{root.pendingRestoreSource=selectedFile;restoreConfirmDialog.open()}
    }
    Dialog {
        id:restoreConfirmDialog;title:"Agendar restauração?";modal:true
        standardButtons:Dialog.Ok|Dialog.Cancel
        onAccepted:presentationController.scheduleRestore(root.pendingRestoreSource)
        Label {text:"O banco atual será preservado em um backup de segurança. A restauração será aplicada somente após reiniciar o app.";wrapMode:Text.WordWrap;width:420}
    }

    FileDialog {
        id: audioDialog
        title: "Importar áudios"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Áudios (*.mp3 *.wav *.flac *.m4a *.aac *.ogg *.opus *.wma *.aiff *.aif)"]
        onAccepted: presentationController.importAudioFiles(selectedFiles)
    }

    FileDialog {
        id: videoDialog
        title: "Importar vídeos"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Vídeos (*.mp4 *.mov *.m4v *.mkv *.webm *.avi *.wmv *.mpeg *.mpg)"]
        onAccepted: presentationController.importVideoFiles(selectedFiles)
    }

    FileDialog {
        id: imageDialog
        title: "Importar imagens"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Imagens (*.jpg *.jpeg *.png *.webp *.bmp *.gif *.tif *.tiff *.heic)"]
        onAccepted: presentationController.importImageFiles(selectedFiles)
    }
    FileDialog {
        id: bibleImportDialog
        title: "Importar JSON HolyScreen legado"
        nameFilters: ["HolyScreen Bíblia JSON (*.json)"]
        onAccepted: presentationController.importBibleTranslation(selectedFile)
    }
    FolderDialog {
        id: bibleFolderDialog
        title: "Selecionar repositório, data/canonical ou pasta da tradução"
        onAccepted: presentationController.importBibleFolder(selectedFolder)
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
                    enabled: !presentationController.bibleImportRunning && bibleGitUrl.text.trim().length > 0
                    onClicked: {
                        if (presentationController.importBibleGit(bibleGitUrl.text))
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
                    enabled: !presentationController.bibleImportRunning && bibleZipUrl.text.trim().length > 0
                    onClicked: {
                        if (presentationController.importBibleZip(bibleZipUrl.text))
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
        onAccepted: presentationController.confirmBibleImportLicenses()
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
                text: presentationController.bibleImportLicenseWarning
                wrapMode: Text.WordWrap
                color: "#eff6ff"
                font.bold: true
            }
        }
    }
    Connections {
        target: presentationController
        function onBibleImportStateChanged() {
            if (presentationController.bibleImportRequiresLicenseConfirmation
                    && !presentationController.bibleImportRunning
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
                    enabled: !presentationController.bibleImportRunning
                    onClicked: bibleFolderDialog.open()
                }
                Button {
                    text: "GIT / ZIP"
                    enabled: !presentationController.bibleImportRunning
                    onClicked: bibleOnlineImportDialog.open()
                }
                Button {
                    text: "JSON LEGADO"
                    enabled: !presentationController.bibleImportRunning
                    onClicked: bibleImportDialog.open()
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                visible: presentationController.bibleImportRunning
                         || presentationController.bibleImportMessage.length > 0
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        text: presentationController.bibleImportMessage
                        color: presentationController.bibleImportRunning ? "#70e1a7" : "#b8c6dc"
                        elide: Text.ElideRight
                    }
                    Button {
                        text: "CANCELAR"
                        visible: presentationController.bibleImportRunning
                        onClicked: presentationController.cancelBibleImport()
                    }
                }
                ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 100
                    value: presentationController.bibleImportProgress
                    indeterminate: presentationController.bibleImportRunning
                                   && presentationController.bibleImportProgress === 0
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
                    model: presentationController.bibleTranslations
                    textRole: "displayName"; valueRole: "id"
                    currentIndex: root.translationIndex(model, presentationController.biblePrimaryTranslationId)
                    onActivated: presentationController.biblePrimaryTranslationId = currentValue
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: root.optionalBibleTranslations
                    textRole: "displayName"; valueRole: "id"
                    currentIndex: root.translationIndex(model, presentationController.bibleSecondaryTranslationId)
                    onActivated: presentationController.bibleSecondaryTranslationId = currentValue
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: root.optionalBibleTranslations
                    textRole: "displayName"; valueRole: "id"
                    currentIndex: root.translationIndex(model, presentationController.bibleTertiaryTranslationId)
                    onActivated: presentationController.bibleTertiaryTranslationId = currentValue
                }
            }
            RowLayout {
                Layout.fillWidth: true
                visible: presentationController.bibleTranslations.length > 0
                Label { text: "ORIGEM:"; color: "#8da0bc"; font.bold: true }
                ComboBox {
                    id: bibleManagedTranslation
                    Layout.fillWidth: true
                    model: presentationController.bibleTranslations
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
                    enabled: !presentationController.bibleImportRunning
                             && bibleManagedTranslation.currentIndex >= 0
                             && !!bibleManagedTranslation.model[bibleManagedTranslation.currentIndex].canUpdate
                    onClicked: presentationController.updateBibleTranslationFromSource(
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
                    text: presentationController.bibleReferenceInput
                    onTextEdited: presentationController.bibleReferenceInput = text
                    onAccepted: presentationController.searchBibleReference()
                }
                Button { text: "BUSCAR"; highlighted: true; onClicked: presentationController.searchBibleReference() }
            }
            Label {
                visible: presentationController.bibleTranslations.length === 0
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
                model: presentationController.bibleResults
                delegate: Rectangle {
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
                            Label { text: modelData.label; color: "#70e1a7"; font.bold: true }
                            Label {
                                id: bibleResultText
                                Layout.fillWidth: true
                                text: modelData.text
                                color: "#eff6ff"
                                wrapMode: Text.WordWrap
                            }
                        }
                        Button { text: "APRESENTAR"; onClicked: presentationController.showBibleVerse(index) }
                    }
                }
                Label {
                    anchors.centerIn: parent
                    visible: bibleResultsList.count === 0 && presentationController.bibleTranslations.length > 0
                    text: "Digite uma referência para localizar os versículos"
                    color: "#64748b"
                }
            }
        }
    }
    FolderDialog {
        id: mediaFolderDialog
        title: "Adicionar pasta de mídia"
        onAccepted: presentationController.addMediaFolder(selectedFolder)
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
                Button { text: "ATUALIZAR"; onClicked: presentationController.rescanMediaFolders() }
            }
            ListView {
                id: mediaFoldersList
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(120, Math.max(42, contentHeight))
                clip: true
                spacing: 4
                model: presentationController.mediaFolders
                delegate: Rectangle {
                    required property var modelData
                    width: ListView.view.width
                    height: 38
                    radius: 5
                    color: "#142137"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 6
                        Label { text: modelData.exists ? "●" : "!"; color: modelData.exists ? "#70e1a7" : "#ffba70" }
                        Label { Layout.fillWidth: true; text: modelData.path; color: "#d9e5f5"; elide: Text.ElideMiddle }
                        ToolButton { text: "×"; onClicked: presentationController.removeMediaFolder(modelData.path) }
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
                TabButton { text: "ÁUDIOS (" + presentationController.folderAudioFiles.length + ")" }
                TabButton { text: "VÍDEOS (" + presentationController.folderVideoFiles.length + ")" }
                TabButton { text: "IMAGENS (" + presentationController.folderImageFiles.length + ")" }
            }
            TextField {
                Layout.fillWidth: true
                placeholderText: "Pesquisar por nome de arquivo"
                text: mediaTypeTabs.currentIndex === 0 ? presentationController.audioFileSearch
                      : mediaTypeTabs.currentIndex === 1 ? presentationController.videoFileSearch
                      : presentationController.imageFileSearch
                onTextEdited: {
                    if (mediaTypeTabs.currentIndex === 0) presentationController.audioFileSearch = text
                    else if (mediaTypeTabs.currentIndex === 1) presentationController.videoFileSearch = text
                    else presentationController.imageFileSearch = text
                }
            }
            ListView {
                id: folderMediaList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 4
                model: mediaTypeTabs.currentIndex === 0 ? presentationController.folderAudioFiles
                     : mediaTypeTabs.currentIndex === 1 ? presentationController.folderVideoFiles
                     : presentationController.folderImageFiles
                delegate: Rectangle {
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
                            text: modelData.fileName
                            color: "#eff6ff"
                            elide: Text.ElideMiddle
                        }
                        Label {
                            text: modelData.folderPath
                            color: "#7185a3"
                            elide: Text.ElideMiddle
                            Layout.maximumWidth: 260
                        }
                        Button {
                            text: modelData.inPlaylist ? "NA PLAYLIST" : "+ PLAYLIST"
                            enabled: !modelData.inPlaylist
                            onClicked: presentationController.addCatalogFileToPlaylist(modelData.path)
                        }
                    }
                }
                Label {
                    anchors.centerIn: parent
                    visible: folderMediaList.count === 0
                    text: presentationController.mediaFolders.length === 0
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
        onAccepted: presentationController.updateTheme({backgroundType: 2, backgroundImage: selectedFile.toString()})
    }

    menuBar: MenuBar {
        visible: presentationController.debugEnabled
        Menu {
            title: "Debug"
            MenuItem {
                text: "Ativar modo de debug"
                checkable: true
                checked: presentationController.debugEnabled
                onTriggered: presentationController.debugEnabled = checked
            }
            MenuSeparator { }
            MenuItem {
                text: "Múltiplas saídas simuladas"
                checkable: true
                enabled: presentationController.debugEnabled
                checked: presentationController.debugSimulatedOutputs
                onTriggered: presentationController.debugSimulatedOutputs = checked
            }
            MenuItem {
                text: "Diagnósticos e benchmark"
                checkable: true
                enabled: presentationController.debugEnabled
                checked: presentationController.debugDiagnostics
                onTriggered: presentationController.debugDiagnostics = checked
            }
            MenuItem {
                text: "Registrar mensagens DEBUG"
                checkable: true
                enabled: presentationController.debugEnabled
                checked: presentationController.debugLogging
                onTriggered: presentationController.debugLogging = checked
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
                Item { Layout.fillWidth: true }
                ToolButton {
                    text: "⚙"
                    Accessible.name: "Configurações"
                    onClicked: settingsDialog.open()
                }
                ToolButton { text: "⛶"; onClicked: root.visibility = root.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
                Button { text: presentationController.blackout ? "Restaurar" : "Ao vivo"; highlighted: true; onClicked: presentationController.setBlackout(false) }
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
                        model: presentationController.favoriteMedia
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
        controller: presentationController
    }
    QuickBibleSearch {
        id: quickBibleSearch
        controller: presentationController
    }
    Connections {
        target: presentationController
        function onQuickBibleSearchRequested(initialText) {
            if (!quickBibleSearch.visible)
                quickBibleSearch.openWithText(initialText)
        }
    }

    Dashboard {
        anchors.fill: parent
        controller: presentationController
        onOpenLibrary: mediaLibraryDialog.open()
        onOpenBible: bibleDialog.open()
        onOpenBibleBrowser: bibleBrowser.open()
        onImportAudio: audioDialog.open()
        onImportVideo: videoDialog.open()
        onImportImage: imageDialog.open()
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
                            model: presentationController.screens
                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: modelData.selected
                                                ? (presentationController.bibleTranslations.length > 0 ? 176 : 146)
                                                : 80
                                radius: 8
                                color: modelData.selected ? "#18345a" : "#152137"
                                border.color: modelData.selected ? "#3b82f6" : "#263852"
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    CheckBox {
                                        checked: modelData.selected
                                        enabled: !modelData.primary
                                        // `toggled` also reacts to model-driven checked changes
                                        // while delegates are rebuilt. Only user clicks should
                                        // mutate the configured output list.
                                        onClicked: presentationController.toggleScreen(modelData.id, checked)
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        Label {
                                            text: modelData.name
                                            color: "#eff6ff"
                                            font.bold: true
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                        Label {
                                            text: modelData.primary ? "OPERADOR • tela principal"
                                                  : modelData.selected && modelData.role === "stage" ? "PALCO"
                                                  : "DISPONÍVEL"
                                            color: "#8da0bc"
                                            font.pixelSize: 11
                                        }
                                        ComboBox {
                                            visible: !modelData.primary && modelData.selected
                                            Layout.fillWidth: true
                                            model: root.outputRoles
                                            textRole: "displayName"
                                            valueRole: "id"
                                            currentIndex: root.valueIndex(model, modelData.role)
                                            onActivated: presentationController.setOutputRole(modelData.id, currentValue)
                                        }
                                        CheckBox {
                                            visible: !modelData.primary && modelData.selected
                                            text: "Exibir mídia nesta tela"
                                            checked: modelData.mediaEnabled
                                            onClicked: presentationController.setOutputMediaEnabled(modelData.id, checked)
                                        }
                                        ComboBox {
                                            visible: !modelData.primary && modelData.selected
                                                     && presentationController.bibleTranslations.length > 0
                                            Layout.fillWidth: true
                                            model: root.outputBibleTranslations
                                            textRole: "displayName"
                                            valueRole: "id"
                                            currentIndex: root.translationIndex(model, modelData.bibleTranslationId)
                                            onActivated: presentationController.setOutputBibleTranslation(modelData.id, currentValue)
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
                    onClicked: presentationController.enableAllScreens()
                }
                Button {
                    text: "IDENTIFICAR TELAS"
                    Layout.fillWidth: true
                    onClicked: presentationController.identifyScreens()
                }
                Label {
                    visible: presentationController.statusMessage.length > 0
                    text: presentationController.statusMessage
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
                            required property int index
                            Layout.minimumWidth: 220
                            Layout.minimumHeight: root.previewOutputCount > 4 ? 80
                                                  : root.previewOutputCount > 2 ? 110 : 160
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            outputLabel: root.previewOutputCount > 1 ? "SIMULAÇÃO " + (index + 1) : "PRÉVIA"
                            identifier: index + 1
                            wallpaper: presentationController.wallpaperColor
                            wallpaperSource: presentationController.wallpaperSource
                            wallpaperFit: presentationController.wallpaperFit
                            showClock: presentationController.clockVisible
                            clockText: presentationController.clockText
                            clockPosition: presentationController.clockPosition
                            clockFamily: presentationController.clockFontFamily
                            clockColor: presentationController.clockColor
                            isBlackout: presentationController.blackout
                            identifyVisible: presentationController.identifyVisible
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
                                model: presentationController.mediaPlaylist
                                delegate: Rectangle {
                                    id: mediaDelegate
                                    required property var modelData
                                    required property int index
                                    width: ListView.view.width
                                    height: 44
                                    radius: 6
                                    color: presentationController.currentMediaId === modelData.id ? "#18345a" : "#142137"
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 10
                                        anchors.rightMargin: 6
                                        Label {
                                            Layout.fillWidth: true
                                            text: modelData.title
                                            color: "#eff6ff"
                                            elide: Text.ElideRight
                                        }
                                        Label { text: modelData.typeLabel; color: modelData.type === "video" ? "#c4a7ff" : "#70e1a7"; font.pixelSize: 9; font.bold: true }
                                        Label { text: root.formatDuration(modelData.durationMs); color: "#8da0bc"; font.pixelSize: 11 }
                                        ToolButton { text: "↑"; enabled: index > 0; onClicked: presentationController.moveMedia(modelData.id, index - 1) }
                                        ToolButton { text: "↓"; enabled: index + 1 < mediaList.count; onClicked: presentationController.moveMedia(modelData.id, index + 1) }
                                        ToolButton {
                                            text: "×"
                                            onClicked: presentationController.removeMedia(modelData.id)
                                        }
                                    }
                                    TapHandler {
                                        acceptedButtons: Qt.LeftButton
                                        onDoubleTapped: presentationController.playMedia(mediaDelegate.modelData.id)
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
                                text: presentationController.currentMediaTitle.length > 0
                                      ? presentationController.currentMediaTitle : "Nenhuma mídia selecionada"
                                color: "#eff6ff"
                                font.bold: true
                                font.pixelSize: 14
                                elide: Text.ElideRight
                            }
                            Label {
                                text: (presentationController.currentMediaType.length > 0
                                       ? presentationController.currentMediaType.toUpperCase() + " · " : "")
                                      + presentationController.mediaState.toUpperCase()
                                color: presentationController.mediaState === "playing" ? "#70e1a7" : "#8da0bc"
                                font.pixelSize: 10
                                font.bold: true
                            }
                            Slider {
                                Layout.fillWidth: true
                                from: 0
                                to: Math.max(1, presentationController.mediaDurationMs)
                                value: presentationController.mediaPositionMs
                                onMoved: presentationController.seekMedia(value)
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: root.formatDuration(presentationController.mediaPositionMs); color: "#8da0bc" }
                                Item { Layout.fillWidth: true }
                                Label { text: root.formatDuration(presentationController.mediaDurationMs); color: "#8da0bc" }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Button { text: "ANTERIOR"; onClicked: presentationController.previousMedia() }
                                Button {
                                    text: presentationController.mediaState === "playing" ? "PAUSAR" : "TOCAR"
                                    highlighted: true
                                    onClicked: {
                                        if (presentationController.currentMediaId.length === 0 && mediaList.count > 0)
                                            presentationController.playMedia(presentationController.mediaPlaylist[0].id)
                                        else
                                            presentationController.toggleMediaPause()
                                    }
                                }
                                Button { text: "PARAR"; onClicked: presentationController.stopMedia() }
                                Button { text: "PRÓXIMO"; onClicked: presentationController.nextMedia() }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: "VOLUME"; color: "#8da0bc"; font.pixelSize: 10 }
                                Slider {
                                    Layout.fillWidth: true
                                    from: 0; to: 1; stepSize: 0.01
                                    value: presentationController.mediaVolume
                                    onMoved: presentationController.mediaVolume = value
                                }
                                ComboBox {
                                    model: ["Não repetir", "Repetir uma", "Repetir playlist"]
                                    currentIndex: presentationController.mediaRepeatMode === "one" ? 1
                                                  : presentationController.mediaRepeatMode === "all" ? 2 : 0
                                    onActivated: presentationController.mediaRepeatMode = currentIndex === 1 ? "one"
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
                                required property string modelData
                                width: 48; height: 34; radius: 7; color: modelData
                                border.width: presentationController.wallpaperColor === modelData ? 3 : 1
                                border.color: presentationController.wallpaperColor === modelData ? "#f8fafc" : "#3b4c66"
                                TapHandler { onTapped: presentationController.wallpaperColor = modelData }
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: "ESCOLHER IMAGEM"; Layout.fillWidth: true; onClicked: wallpaperDialog.open() }
                        Button { text: "LIMPAR"; onClicked: presentationController.wallpaperSource = "" }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["cover", "contain", "stretch", "center"]
                        currentIndex: Math.max(0, model.indexOf(presentationController.wallpaperFit))
                        onActivated: presentationController.wallpaperFit = currentText
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { text: "RELÓGIO"; color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
                    CheckBox {
                        text: "Exibir relógio"
                        checked: presentationController.clockVisible
                        onToggled: presentationController.clockVisible = checked
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["bottomRight", "topRight", "bottomLeft", "topLeft"]
                        currentIndex: Math.max(0, model.indexOf(presentationController.clockPosition))
                        onActivated: presentationController.clockPosition = currentText
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["24h", "24h-seconds", "12h"]
                        currentIndex: Math.max(0, model.indexOf(presentationController.clockFormat))
                        onActivated: presentationController.clockFormat = currentText
                    }
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "Fonte"
                        text: presentationController.clockFontFamily
                        onEditingFinished: presentationController.clockFontFamily = text
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "Tamanho"; color: "#c8d5e8" }
                        SpinBox {
                            from: 16; to: 240
                            value: presentationController.clockFontSize
                            onValueModified: presentationController.clockFontSize = value
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
                            onClicked: presentationController.removeImage(imagePicker.currentValue)
                        }
                    }
                    ComboBox {
                        id: imagePicker
                        Layout.fillWidth: true
                        model: presentationController.imageLibrary
                        textRole: "title"
                        valueRole: "id"
                        displayText: presentationController.currentImageTitle.length > 0
                                     ? presentationController.currentImageTitle
                                     : (currentText.length > 0 ? currentText : "Nenhuma imagem")
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        ToolButton {
                            text: "↑"
                            enabled: imagePicker.currentIndex > 0
                            onClicked: presentationController.moveImage(imagePicker.currentValue, imagePicker.currentIndex - 1)
                        }
                        ToolButton {
                            text: "↓"
                            enabled: imagePicker.currentIndex >= 0 && imagePicker.currentIndex + 1 < imagePicker.count
                            onClicked: presentationController.moveImage(imagePicker.currentValue, imagePicker.currentIndex + 1)
                        }
                        Button {
                            text: "EXIBIR"
                            Layout.fillWidth: true
                            highlighted: true
                            enabled: imagePicker.currentValue !== undefined && imagePicker.currentValue.length > 0
                            onClicked: presentationController.showImage(imagePicker.currentValue)
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: "ANTERIOR"; Layout.fillWidth: true; onClicked: presentationController.previousImage() }
                        Button { text: "PRÓXIMA"; Layout.fillWidth: true; onClicked: presentationController.nextImage() }
                        Button { text: "PARAR"; onClicked: presentationController.stopImage() }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["contain", "cover", "stretch", "center"]
                            currentIndex: Math.max(0, model.indexOf(presentationController.imageFit))
                            onActivated: presentationController.imageFit = currentText
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["fade", "none"]
                            currentIndex: Math.max(0, model.indexOf(presentationController.imageTransition))
                            onActivated: presentationController.imageTransition = currentText
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        CheckBox {
                            text: "Autoplay"
                            checked: presentationController.imageAutoplay
                            onToggled: presentationController.imageAutoplay = checked
                        }
                        Label { text: "segundos"; color: "#8da0bc"; font.pixelSize: 10 }
                        SpinBox {
                            from: 1; to: 3600
                            value: Math.round(presentationController.imageIntervalMs / 1000)
                            onValueModified: presentationController.imageIntervalMs = value * 1000
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
                                    presentationController.createTextPresentation(newPresentationTitle.text)
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
                            model: presentationController.textPresentations
                            textRole: "title"; valueRole: "id"
                            onActivated: presentationController.selectTextPresentation(currentValue)
                        }
                        Button {
                            text: "EXCLUIR"
                            enabled: presentationController.currentPresentationId.length > 0
                            onClicked: presentationController.deleteTextPresentation(presentationController.currentPresentationId)
                        }
                    }
                    ComboBox {
                        id: slidePicker
                        Layout.fillWidth: true
                        model: presentationController.textSlides
                        textRole: "label"
                        currentIndex: Math.max(0, presentationController.currentSlideIndex)
                        onActivated: presentationController.showTextSlide(currentIndex)
                    }
                    TextField {
                        id: slideLabelEditor
                        Layout.fillWidth: true
                        placeholderText: "Rótulo do slide"
                        text: presentationController.currentSlideLabel
                    }
                    TextArea {
                        id: slideTextEditor
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        placeholderText: "Texto do slide"
                        text: presentationController.currentSlideText
                        wrapMode: TextEdit.Wrap
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            text: "SALVAR"
                            Layout.fillWidth: true
                            enabled: presentationController.currentSlideId.length > 0
                            onClicked: presentationController.updateTextSlide(presentationController.currentSlideId, slideLabelEditor.text, slideTextEditor.text)
                        }
                        Button {
                            text: "+ SLIDE"
                            enabled: presentationController.currentPresentationId.length > 0
                            onClicked: presentationController.addTextSlide(String(presentationController.textSlides.length + 1), "Novo slide")
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: "DUPLICAR"; enabled: presentationController.currentSlideId.length > 0; onClicked: presentationController.duplicateTextSlide(presentationController.currentSlideId) }
                        Button { text: "DIVIDIR"; enabled: presentationController.currentSlideId.length > 0; onClicked: presentationController.splitTextSlide(presentationController.currentSlideId, slideTextEditor.cursorPosition) }
                        Button { text: "EXCLUIR"; enabled: presentationController.currentSlideId.length > 0; onClicked: presentationController.removeTextSlide(presentationController.currentSlideId) }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        ToolButton { text: "↑"; enabled: slidePicker.currentIndex > 0; onClicked: presentationController.moveTextSlide(presentationController.currentSlideId, slidePicker.currentIndex - 1) }
                        ToolButton { text: "↓"; enabled: slidePicker.currentIndex + 1 < slidePicker.count; onClicked: presentationController.moveTextSlide(presentationController.currentSlideId, slidePicker.currentIndex + 1) }
                        Button { text: "ANTERIOR"; Layout.fillWidth: true; onClicked: presentationController.previousTextSlide() }
                        Button { text: "EXIBIR"; highlighted: true; enabled: slidePicker.currentIndex >= 0; onClicked: presentationController.showTextSlide(slidePicker.currentIndex) }
                        Button { text: "PRÓXIMO"; Layout.fillWidth: true; onClicked: presentationController.nextTextSlide() }
                    }
                    Button { text: "VOLTAR AO WALLPAPER"; Layout.fillWidth: true; onClicked: presentationController.stopTextPresentation() }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { text: "TEMAS"; color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField { id: newThemeName; Layout.fillWidth: true; placeholderText: "Nome do tema" }
                        Button { text: "NOVO"; onClicked: { if(newThemeName.text.trim().length>0){presentationController.createTheme(newThemeName.text);newThemeName.clear()} } }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        ComboBox { id: themePicker; Layout.fillWidth: true; model: presentationController.themes; textRole:"name"; valueRole:"id"; onActivated: presentationController.applyTheme(currentValue) }
                        Button { text:"APLICAR"; enabled:themePicker.currentValue!==undefined; onClicked:presentationController.applyTheme(themePicker.currentValue) }
                        Button { text:"EXCLUIR"; enabled:presentationController.themes.length>1; onClicked:presentationController.deleteTheme(themePicker.currentValue) }
                    }
                    TextField { Layout.fillWidth:true; placeholderText:"Família da fonte"; text:presentationController.activeTheme.fontFamily || ""; onEditingFinished:presentationController.updateTheme({fontFamily:text}) }
                    RowLayout {
                        Layout.fillWidth:true
                        Label { text:"Fonte"; color:"#8da0bc" }
                        SpinBox { from:28;to:240;value:presentationController.activeTheme.fontSize || 72; onValueModified:presentationController.updateTheme({fontSize:value}) }
                        Label { text:"Margem"; color:"#8da0bc" }
                        SpinBox { from:0;to:400;value:presentationController.activeTheme.margin || 64; onValueModified:presentationController.updateTheme({margin:value}) }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        TextField { Layout.fillWidth:true; placeholderText:"Cor do texto"; text:presentationController.activeTheme.textColor || "#ffffff"; onEditingFinished:presentationController.updateTheme({textColor:text}) }
                        TextField { Layout.fillWidth:true; placeholderText:"Cor de fundo"; text:presentationController.activeTheme.backgroundColor || "#000000"; onEditingFinished:presentationController.updateTheme({backgroundColor:text,backgroundType:0}) }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        Button { text:"FUNDO COM IMAGEM"; Layout.fillWidth:true; onClicked:themeBackgroundDialog.open() }
                        Button { text:"FUNDO SÓLIDO"; onClicked:presentationController.updateTheme({backgroundType:0}) }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        ComboBox { Layout.fillWidth:true; model:["left","center","right"]; currentIndex:Math.max(0,model.indexOf(presentationController.activeTheme.horizontalAlignment)); onActivated:presentationController.updateTheme({horizontalAlignment:currentText}) }
                        ComboBox { Layout.fillWidth:true; model:["top","center","bottom"]; currentIndex:Math.max(0,model.indexOf(presentationController.activeTheme.verticalAlignment)); onActivated:presentationController.updateTheme({verticalAlignment:currentText}) }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        CheckBox { text:"Contorno"; checked:presentationController.activeTheme.outline || false; onToggled:presentationController.updateTheme({outline:checked}) }
                        CheckBox { text:"Sombra"; checked:presentationController.activeTheme.shadow || false; onToggled:presentationController.updateTheme({shadow:checked}) }
                        ComboBox { Layout.fillWidth:true; model:["fade","none"]; currentIndex:Math.max(0,model.indexOf(presentationController.activeTheme.transition)); onActivated:presentationController.updateTheme({transition:currentText}) }
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { text:"MÚSICAS"; color:"#8da0bc"; font.bold:true; font.pixelSize:11 }
                    TextField { Layout.fillWidth:true; placeholderText:"Buscar por título, autor ou letra"; text:presentationController.songSearch; onTextEdited:presentationController.songSearch=text }
                    RowLayout {
                        Layout.fillWidth:true
                        ComboBox { id:songPicker; Layout.fillWidth:true; model:presentationController.songs; textRole:"title"; valueRole:"id"; onActivated:presentationController.selectSong(currentValue) }
                        Button { text:"ABRIR"; enabled:songPicker.currentValue!==undefined; onClicked:presentationController.selectSong(songPicker.currentValue) }
                        Button { text:"EXCLUIR"; enabled:songPicker.currentValue!==undefined; onClicked:presentationController.deleteTextPresentation(songPicker.currentValue) }
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
                                presentationController.createSong(newSongTitle.text,newSongAuthor.text,newSongLyrics.text,newSongSequence.text)
                                newSongTitle.clear();newSongAuthor.clear();newSongLyrics.clear();newSongSequence.clear()
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        TextField { id:editSongSequence; Layout.fillWidth:true; placeholderText:"Sequência atual"; text:presentationController.songSequence }
                        Button { text:"SALVAR SEQUÊNCIA"; enabled:presentationController.currentPresentationId.length>0; onClicked:presentationController.updateSongSequence(editSongSequence.text) }
                    }

                    Rectangle { Layout.fillWidth:true; Layout.preferredHeight:1; color:"#24334b" }
                    Label { text:"PLAYLIST DE CULTO"; color:"#8da0bc"; font.bold:true; font.pixelSize:11 }
                    RowLayout {
                        Layout.fillWidth:true
                        TextField { id:newEventTitle; Layout.fillWidth:true; placeholderText:"Nome do culto" }
                        TextField { id:newEventDate; Layout.preferredWidth:120; placeholderText:"Data/hora" }
                        Button { text:"NOVO"; onClicked:{if(newEventTitle.text.trim().length>0){presentationController.createEvent(newEventTitle.text,newEventDate.text);newEventTitle.clear();newEventDate.clear()}} }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        ComboBox { id:eventPicker; Layout.fillWidth:true; model:presentationController.events; textRole:"title";valueRole:"id";onActivated:presentationController.selectEvent(currentValue) }
                        Button { text:"ABRIR"; enabled:eventPicker.currentValue!==undefined;onClicked:presentationController.selectEvent(eventPicker.currentValue) }
                        Button { text:"EXCLUIR";enabled:presentationController.currentEventId.length>0;onClicked:presentationController.deleteEvent(presentationController.currentEventId) }
                    }
                    Label { text:"Duração total: "+root.formatDuration(presentationController.eventDurationMs);color:"#c8d5e8" }
                    Flow {
                        Layout.fillWidth:true;spacing:5
                        Button { text:"+ APRESENTAÇÃO";enabled:presentationController.currentEventId.length>0&&presentationController.currentPresentationId.length>0;onClicked:presentationController.addEventItem(presentationController.currentPresentationType,presentationController.currentPresentationId,presentationController.currentPresentationTitle,0) }
                        Button { text:"+ IMAGEM";enabled:presentationController.currentEventId.length>0&&presentationController.currentImageId.length>0;onClicked:presentationController.addEventItem("image",presentationController.currentImageId,presentationController.currentImageTitle,0) }
                        Button { text:"+ VÍDEO";enabled:presentationController.currentEventId.length>0&&presentationController.currentVideoId.length>0;onClicked:presentationController.addEventItem("video",presentationController.currentVideoId,presentationController.currentVideoTitle,presentationController.videoDurationMs) }
                        Button { text:"+ ÁUDIO";enabled:presentationController.currentEventId.length>0&&presentationController.currentAudioId.length>0;onClicked:presentationController.addEventItem("audio",presentationController.currentAudioId,presentationController.currentAudioTitle,presentationController.audioDurationMs) }
                    }
                    ListView {
                        id:eventItemsList;Layout.fillWidth:true;Layout.preferredHeight:Math.min(260,Math.max(60,contentHeight));clip:true;spacing:4
                        model:presentationController.eventItems
                        delegate:Rectangle {
                            id:eventItemDelegate;required property var modelData;required property int index
                            width:ListView.view.width;height:46;color:"#142137";radius:6
                            RowLayout { anchors.fill:parent;anchors.margins:5
                                Label { text:"⋮⋮";color:"#8da0bc"
                                    DragHandler { target:null;onActiveChanged:{if(!active){const targetIndex=Math.max(0,Math.min(eventItemsList.count-1,eventItemDelegate.index+Math.round(translation.y/eventItemDelegate.height)));if(targetIndex!==eventItemDelegate.index)presentationController.moveEventItem(eventItemDelegate.modelData.id,targetIndex)}} }
                                }
                                Label { text:modelData.type.toUpperCase();color:"#70e1a7";font.pixelSize:9 }
                                Label { Layout.fillWidth:true;text:modelData.title;color:"#eff6ff";elide:Text.ElideRight }
                                Label { text:root.formatDuration(modelData.durationMs);color:"#8da0bc";font.pixelSize:10 }
                                ToolButton { text:"↑";enabled:index>0;onClicked:presentationController.moveEventItem(modelData.id,index-1) }
                                ToolButton { text:"↓";enabled:index+1<eventItemsList.count;onClicked:presentationController.moveEventItem(modelData.id,index+1) }
                                Button { text:"EXECUTAR";onClicked:presentationController.executeEventItem(modelData.id) }
                                ToolButton { text:"×";onClicked:presentationController.removeEventItem(modelData.id) }
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth:true;Layout.preferredHeight:1;color:"#24334b" }
                    RowLayout {
                        Layout.fillWidth:true
                        Label { text:"HISTÓRICO";color:"#8da0bc";font.bold:true;font.pixelSize:11 }
                        Item { Layout.fillWidth:true }
                        Button { text:"LIMPAR";enabled:presentationController.history.length>0;onClicked:clearHistoryDialog.open() }
                    }
                    Label {
                        Layout.fillWidth:true
                        text:"Execuções: "+(presentationController.historyReport.totalExecutions||0)+"  •  Mais executado: "+(presentationController.historyReport.mostExecutedTitle||"—")
                        color:"#c8d5e8";wrapMode:Text.WordWrap
                    }
                    ListView {
                        Layout.fillWidth:true;Layout.preferredHeight:Math.min(220,Math.max(50,contentHeight));clip:true;spacing:3
                        model:presentationController.history
                        delegate:Rectangle {
                            required property var modelData;width:ListView.view.width;height:40;color:"#142137";radius:5
                            RowLayout {anchors.fill:parent;anchors.margins:6
                                Label {text:modelData.type.toUpperCase();color:"#70e1a7";font.pixelSize:9}
                                Label {Layout.fillWidth:true;text:modelData.title;color:"#eff6ff";elide:Text.ElideRight}
                                Label {text:modelData.executedAt.replace("T"," ").slice(0,16);color:"#8da0bc";font.pixelSize:9}
                            }
                        }
                    }

                    Rectangle {Layout.fillWidth:true;Layout.preferredHeight:1;color:"#24334b"}
                    Label {text:"MANUTENÇÃO E DIAGNÓSTICOS";color:"#8da0bc";font.bold:true;font.pixelSize:11}
                    Label {visible:presentationController.recoveredFromCrash;text:"Uma sessão anterior terminou inesperadamente. Um snapshot de recuperação foi criado.";color:"#ffba70";wrapMode:Text.WordWrap;Layout.fillWidth:true}
                    RowLayout {
                        Layout.fillWidth:true
                        Button {text:"CRIAR BACKUP";Layout.fillWidth:true;onClicked:presentationController.createBackup()}
                        Button {text:"RESTAURAR";Layout.fillWidth:true;onClicked:restoreDialog.open()}
                        Button {visible:presentationController.debugEnabled && presentationController.debugDiagnostics;text:"BENCHMARK";Layout.fillWidth:true;onClicked:presentationController.runBenchmark()}
                    }
                    Label {visible:presentationController.debugEnabled && presentationController.debugDiagnostics;Layout.fillWidth:true;wrapMode:Text.WordWrap;color:"#c8d5e8";text:"Versão "+(presentationController.diagnostics.version||"—")+" • Qt "+(presentationController.diagnostics.qtVersion||"—")+" • "+(presentationController.diagnostics.platform||"—")+" • Telas "+(presentationController.diagnostics.detectedScreens||0)+" • Ops/s "+(presentationController.diagnostics.benchmarkOperationsPerSecond||"—")}
                    TextField {Layout.fillWidth:true;placeholderText:"URL HTTPS do manifesto de atualização";text:presentationController.updateEndpoint;onEditingFinished:presentationController.updateEndpoint=text}
                    RowLayout {Layout.fillWidth:true
                        Button {text:"VERIFICAR ATUALIZAÇÕES";onClicked:presentationController.checkForUpdates()}
                        Label {Layout.fillWidth:true;text:presentationController.updateStatus;color:"#8da0bc";wrapMode:Text.WordWrap}
                    }

                    Rectangle { visible:presentationController.debugEnabled;Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
                    Label { visible:presentationController.debugEnabled;text: "DESENVOLVIMENTO"; color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
                    Label {
                        visible: presentationController.debugEnabled && presentationController.debugSimulatedOutputs
                        text: "Saídas simuladas: " + presentationController.simulatedOutputCount
                        color: "#c8d5e8"
                        font.pixelSize: 12
                    }
                    Slider {
                        visible: presentationController.debugEnabled && presentationController.debugSimulatedOutputs
                        Layout.fillWidth: true
                        from: 1; to: 5; stepSize: 1
                        value: presentationController.simulatedOutputCount
                        onMoved: presentationController.simulatedOutputCount = Math.round(value)
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            text: presentationController.undoLabel.length > 0
                                  ? "DESFAZER: " + presentationController.undoLabel.toUpperCase()
                                  : "DESFAZER"
                            Layout.fillWidth: true
                            enabled: presentationController.canUndo
                            onClicked: presentationController.undo()
                        }
                        Button {
                            text: presentationController.redoLabel.length > 0
                                  ? "REFAZER: " + presentationController.redoLabel.toUpperCase()
                                  : "REFAZER"
                            Layout.fillWidth: true
                            enabled: presentationController.canRedo
                            onClicked: presentationController.redo()
                        }
                    }
                    Button {
                        text: presentationController.blackout ? "RESTAURAR APRESENTAÇÃO" : "BLACKOUT"
                        Layout.fillWidth: true
                        highlighted: true
                        onClicked: presentationController.setBlackout(!presentationController.blackout)
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
            if (audio.length > 0) presentationController.importAudioFiles(audio)
            if (video.length > 0) presentationController.importVideoFiles(video)
            if (images.length > 0) presentationController.importImageFiles(images)
        }
    }

    Shortcut { sequence: "Right"; enabled: presentationController.textVisible; onActivated: presentationController.nextTextSlide() }
    Shortcut { sequence: "Left"; enabled: presentationController.textVisible; onActivated: presentationController.previousTextSlide() }
    Shortcut { sequence: "Space"; enabled: presentationController.textVisible && !slideTextEditor.activeFocus; onActivated: presentationController.nextTextSlide() }
    Shortcut { sequence: "Home"; enabled: presentationController.textVisible; onActivated: presentationController.firstTextSlide() }
    Shortcut { sequence: "End"; enabled: presentationController.textVisible; onActivated: presentationController.lastTextSlide() }
    Shortcut { sequence: "Escape"; enabled: presentationController.textVisible; onActivated: presentationController.stopTextPresentation() }
    Shortcut { sequence: StandardKey.Undo; enabled: presentationController.canUndo; onActivated: presentationController.undo() }
    Shortcut { sequence: StandardKey.Redo; enabled: presentationController.canRedo; onActivated: presentationController.redo() }
    Shortcut { sequence: "Ctrl+B"; onActivated: presentationController.createBackup() }
    Shortcut { sequence: "Ctrl+S"; enabled: presentationController.currentSlideId.length > 0; onActivated: presentationController.updateTextSlide(presentationController.currentSlideId, slideLabelEditor.text, slideTextEditor.text) }
    Shortcut { sequence: "F5"; onActivated: presentationController.checkForUpdates() }
    Shortcut { sequence: "Ctrl+Shift+O"; onActivated: { root.show(); root.raise(); root.requestActivate() } }
}
