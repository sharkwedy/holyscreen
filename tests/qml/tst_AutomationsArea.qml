import QtQuick
import QtTest
import ChurchPresenter

TestCase {
    id: testCase
    name: "AutomationsArea"
    width: 1100
    height: 800
    visible: true
    when: windowShown

    QtObject {
        id: fakeController
        property var automations: []
        property var automationRuns: []
        property string automationStatus: ""
        property bool automationsEnabled: true
        property bool processActionsEnabled: false
        property var authorizedExecutables: []
        property var automationTriggerTypes: ["slide.changed", "time.local"]
        property var automationActionTypes: ["command", "integration", "process", "wait"]
        property var automationConditionOperations: ["equals", "contains"]

        function saveAutomation(value) {
            return {"accepted": value.actions.length > 0,
                    "errors": value.actions.length > 0 ? [] : ["Adicione uma ação."],
                    "id": value.id || "saved-id"}
        }
        function dryRunAutomation() { return {"status": "dry-run"} }
        function resumeAutomation() { return true }
        function removeAutomation() { return true }
        function setAutomationEnabled() { return true }
        function authorizeExecutable() { return {"accepted": true} }
        function revokeExecutable() { return true }
        function importAutomations() { return {"accepted": true} }
        function exportAutomations() { return {"accepted": true} }
    }

    AutomationsArea {
        id: area
        parent: testCase
        controller: fakeController
    }

    function init() {
        area.newAutomation()
    }

    function test_reordersActionsWithoutLosingParameters() {
        area.draft = Object.assign({}, area.draft, {"actions": [
            {"type": "command", "parameters": {"type": "presentation.slide.next"}},
            {"type": "wait", "parameters": {"milliseconds": 250}}
        ]})
        area.moveAction(0, 1)
        compare(area.draft.actions[0].type, "wait")
        compare(area.draft.actions[0].parameters.milliseconds, 250)
        compare(area.draft.actions[1].parameters.type, "presentation.slide.next")
    }

    function test_editsLocalTimeParametersAndDays() {
        area.updateDraft("triggerType", "time.local")
        area.updateDraft("triggerParameters", {"time": "09:00", "daysOfWeek": [1, 7]})
        area.updateTriggerParameter("time", "19:45")
        area.toggleTriggerDay(3, true)
        area.toggleTriggerDay(1, false)
        compare(area.draft.triggerParameters.time, "19:45")
        compare(area.draft.triggerParameters.daysOfWeek.join(","), "3,7")
    }

    function test_inlineValidationKeepsDraft() {
        area.save()
        compare(area.validationErrors.length, 1)
        compare(area.draft.name, "Nova automação")
    }
}
