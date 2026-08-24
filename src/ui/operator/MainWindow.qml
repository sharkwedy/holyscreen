import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Dialogs
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
        onRestoreBackup: maintenanceDialogs.openRestore()
        onExportDiagnostics: maintenanceDialogs.openDiagnosticsExport()
    }
    OnboardingDialog {
        id: onboardingDialog
        controller: root.controller
        onOpenSettings: function(tabIndex) { settingsDialog.openTab(tabIndex) }
        onOpenBible: bibleBrowser.open()
    }
    LiveCommunicationDialog {
        id: liveDialog
        controller: root.controller
        availableHeight: root.height
    }
    FileDialog {
        id: wallpaperDialog
        title: qsTr("Selecionar wallpaper")
        nameFilters: [qsTr("Imagens (*.jpg *.jpeg *.png *.webp)")]
        onAccepted: root.controller.wallpaperSource = selectedFile
    }
    MaintenanceDialogs {
        id: maintenanceDialogs
        controller: root.controller.maintenanceContext
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
        onImportAudio: mediaImportFlow.openAudio()
        onImportVideo: mediaImportFlow.openVideo()
        onImportImage: mediaImportFlow.openImage()
    }

    Component.onCompleted: {
        width = Math.max(minimumWidth, operatorWindowSettings.savedWidth)
        height = Math.max(minimumHeight, operatorWindowSettings.savedHeight)
    }

    MediaImportFlow {
        id: mediaImportFlow
        anchors.fill: parent
        controller: root.controller.mediaContext
    }

    OperatorShortcuts {
        controller: root.controller
        onQuickBibleRequested: quickBibleSearch.openWithText("")
        onOperatorActivationRequested: {
            root.show()
            root.raise()
            root.requestActivate()
        }
    }
}
