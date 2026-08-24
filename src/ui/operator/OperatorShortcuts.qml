import QtQuick

Item {
    id: shortcuts

    required property var controller
    signal quickBibleRequested()
    signal operatorActivationRequested()

    Shortcut {
        sequence: shortcuts.controller.shortcuts.next
        enabled: shortcuts.controller.textVisible
        onActivated: shortcuts.controller.nextTextSlide()
    }
    Shortcut {
        sequence: shortcuts.controller.shortcuts.previous
        enabled: shortcuts.controller.textVisible
        onActivated: shortcuts.controller.previousTextSlide()
    }
    Shortcut {
        sequence: "Home"
        enabled: shortcuts.controller.textVisible
        onActivated: shortcuts.controller.firstTextSlide()
    }
    Shortcut {
        sequence: "End"
        enabled: shortcuts.controller.textVisible
        onActivated: shortcuts.controller.lastTextSlide()
    }
    Shortcut {
        sequence: shortcuts.controller.shortcuts.stop
        enabled: shortcuts.controller.textVisible
        onActivated: shortcuts.controller.stopTextPresentation()
    }
    Shortcut {
        sequences: [StandardKey.Undo]
        enabled: shortcuts.controller.canUndo
        onActivated: shortcuts.controller.undo()
    }
    Shortcut {
        sequences: [StandardKey.Redo]
        enabled: shortcuts.controller.canRedo
        onActivated: shortcuts.controller.redo()
    }
    Shortcut {
        sequence: shortcuts.controller.shortcuts.blackout
        onActivated: shortcuts.controller.outputContext.blackout =
                     !shortcuts.controller.outputContext.blackout
    }
    Shortcut {
        sequence: shortcuts.controller.shortcuts.quickBible
        onActivated: shortcuts.quickBibleRequested()
    }
    Shortcut {
        sequence: "Ctrl+Shift+B"
        onActivated: shortcuts.controller.maintenanceContext.createBackup()
    }
    Shortcut {
        sequence: "F5"
        onActivated: shortcuts.controller.maintenanceContext.checkForUpdates()
    }
    Shortcut {
        sequence: "Ctrl+Shift+O"
        onActivated: shortcuts.operatorActivationRequested()
    }
}
