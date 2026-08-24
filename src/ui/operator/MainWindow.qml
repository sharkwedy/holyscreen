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

    Settings {
        id: operatorWindowSettings
        category: "OperatorWindow"
        property int savedWidth: 1360
        property int savedHeight: 820
        property var dashboardHorizontalState
        property var dashboardVerticalState
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
        onAccepted: root.controller.mediaContext.importAudioFiles(selectedFiles)
    }

    FileDialog {
        id: videoDialog
        title: qsTr("Importar vídeos")
        fileMode: FileDialog.OpenFiles
        nameFilters: [qsTr("Vídeos (*.mp4 *.mov *.m4v *.mkv *.webm *.avi *.wmv *.mpeg *.mpg)")]
        onAccepted: root.controller.mediaContext.importVideoFiles(selectedFiles)
    }

    FileDialog {
        id: imageDialog
        title: qsTr("Importar imagens")
        fileMode: FileDialog.OpenFiles
        nameFilters: [qsTr("Imagens (*.jpg *.jpeg *.png *.webp *.bmp *.gif *.tif *.tiff *.heic)")]
        onAccepted: root.controller.mediaContext.importImageFiles(selectedFiles)
    }
    BibleSettingsFlow {
        id: bibleSettingsFlow
        controller: root.controller.bibleContext
        availableWidth: root.width
        availableHeight: root.height
    }
    MediaLibraryDialog {
        id: mediaLibraryDialog
        controller: root.controller.mediaContext
        availableWidth: root.width
        availableHeight: root.height
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
        onOpenBible: bibleSettingsFlow.open()
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
            if (audio.length > 0) root.controller.mediaContext.importAudioFiles(audio)
            if (video.length > 0) root.controller.mediaContext.importVideoFiles(video)
            if (images.length > 0) root.controller.mediaContext.importImageFiles(images)
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
