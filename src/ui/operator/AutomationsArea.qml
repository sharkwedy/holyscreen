pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

// Área de automações do operador. A tela monta gatilho, condições e ações e
// pede validação, ensaio e execução ao domínio.
Dialog {
    id: area

    required property var controller

    title: qsTr("Automações")
    modal: true
    width: Math.min(980, parent ? parent.width - 60 : 980)
    height: Math.min(740, parent ? parent.height - 60 : 740)
    anchors.centerIn: parent
    standardButtons: Dialog.Close

    readonly property color panel: "#20252a"
    readonly property color panelHigh: "#2a3036"
    readonly property color line: "#46505a"
    readonly property color textMain: "#f2f4f5"
    readonly property color textMuted: "#aab2b8"

    property string selectedId: ""
    property var draft: ({})
    property var validationErrors: []

    function newAutomation() {
        area.selectedId = ""
        area.validationErrors = []
        area.draft = {"id": "", "name": qsTr("Nova automação"), "enabled": true,
                      "triggerType": area.controller.automationTriggerTypes[0],
                      "triggerParameters": {}, "conditionGroup": "all",
                      "conditions": [], "actions": [],
                      "debounceMs": 0, "budgetMs": 15000, "failureLimit": 5}
    }

    function selectAutomation(id) {
        const found = area.controller.automations.filter(function (item) { return item.id === id })
        if (found.length === 0) {
            area.newAutomation()
            return
        }
        area.selectedId = id
        area.validationErrors = []
        area.draft = JSON.parse(JSON.stringify(found[0]))
    }

    function updateDraft(key, value) {
        const changes = {}
        changes[key] = value
        area.draft = Object.assign({}, area.draft, changes)
    }

    function updateTriggerParameter(key, value) {
        const parameters = Object.assign({}, area.draft.triggerParameters || ({}))
        parameters[key] = value
        area.updateDraft("triggerParameters", parameters)
    }

    function toggleTriggerDay(day, checked) {
        const days = (((area.draft.triggerParameters || ({})).daysOfWeek) || []).slice()
        const index = days.indexOf(day)
        if (checked && index < 0)
            days.push(day)
        else if (!checked && index >= 0)
            days.splice(index, 1)
        days.sort(function (a, b) { return a - b })
        area.updateTriggerParameter("daysOfWeek", days)
    }

    function updateList(key, index, changes) {
        const list = (area.draft[key] || []).slice()
        list[index] = Object.assign({}, list[index], changes)
        area.updateDraft(key, list)
    }

    function appendTo(key, item) {
        const list = (area.draft[key] || []).slice()
        list.push(item)
        area.updateDraft(key, list)
    }

    function removeFrom(key, index) {
        const list = (area.draft[key] || []).slice()
        list.splice(index, 1)
        area.updateDraft(key, list)
    }

    function moveAction(index, offset) {
        const list = (area.draft.actions || []).slice()
        const target = index + offset
        if (target < 0 || target >= list.length) return
        const moved = list[index]
        list[index] = list[target]
        list[target] = moved
        area.updateDraft("actions", list)
    }

    function save() {
        const result = area.controller.saveAutomation(area.draft)
        area.validationErrors = result.errors || []
        if (result.accepted)
            area.selectAutomation(result.id)
    }

    onOpened: {
        if (area.controller.automations.length > 0)
            area.selectAutomation(area.controller.automations[0].id)
        else
            area.newAutomation()
    }

    background: Rectangle {
        color: area.panel
        border.color: area.line
        radius: 8
    }

    contentItem: ColumnLayout {
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Switch {
                text: qsTr("Automações ativas")
                checked: area.controller.automationsEnabled
                palette.windowText: area.textMain
                onClicked: area.controller.automationsEnabled = checked
            }
            Label {
                visible: !area.controller.automationsEnabled
                text: qsTr("Todas as automações estão pausadas.")
                color: "#ffba70"
                font.pixelSize: UiScale.px(11)
            }
            Item { Layout.fillWidth: true }
            Button { text: qsTr("Importar"); onClicked: importDialog.open() }
            Button { text: qsTr("Exportar"); onClicked: exportDialog.open() }
            Button { text: qsTr("Processos autorizados"); onClicked: processDialog.open() }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            ColumnLayout {
                Layout.preferredWidth: 270
                Layout.fillHeight: true
                spacing: 6
                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Pesquisar")
                    }
                    Button {
                        text: "+"
                        Accessible.name: qsTr("Nova automação")
                        onClicked: area.newAutomation()
                    }
                }
                ListView {
                    id: automationList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 4
                    model: area.controller.automations.filter(function (item) {
                        const term = searchField.text.toLowerCase()
                        return term.length === 0 || item.name.toLowerCase().indexOf(term) >= 0
                               || item.triggerType.indexOf(term) >= 0
                    })
                    delegate: Rectangle {
                        id: automationRow
                        required property var modelData
                        width: ListView.view.width
                        height: 58
                        radius: 6
                        color: automationRow.modelData.id === area.selectedId ? "#2f3a4b"
                                                                              : area.panelHigh
                        border.color: automationRow.modelData.id === area.selectedId
                                      ? "#7294ff" : area.line
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0
                                Label {
                                    Layout.fillWidth: true
                                    text: automationRow.modelData.name
                                    color: area.textMain
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                                Label {
                                    text: automationRow.modelData.triggerType + " • "
                                          + qsTr("%1 ações").arg(automationRow.modelData.actionCount)
                                    color: area.textMuted
                                    font.pixelSize: UiScale.px(10)
                                }
                                Label {
                                    visible: automationRow.modelData.consecutiveFailures > 0
                                    text: qsTr("%1 falhas seguidas")
                                          .arg(automationRow.modelData.consecutiveFailures)
                                    color: "#ffba70"
                                    font.pixelSize: UiScale.px(10)
                                }
                            }
                            Switch {
                                checked: automationRow.modelData.enabled
                                Accessible.name: qsTr("Ativar automação")
                                onClicked: area.controller.setAutomationEnabled(
                                               automationRow.modelData.id, checked)
                            }
                        }
                        TapHandler {
                            onTapped: area.selectAutomation(automationRow.modelData.id)
                        }
                    }
                    Label {
                        anchors.centerIn: parent
                        visible: automationList.count === 0
                        text: qsTr("Nenhuma automação configurada")
                        color: area.textMuted
                        font.pixelSize: UiScale.px(12)
                    }
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ColumnLayout {
                    width: parent.width
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            Layout.fillWidth: true
                            text: area.draft.name || ""
                            placeholderText: qsTr("Nome")
                            onEditingFinished: area.updateDraft("name", text)
                        }
                    }

                    Label { text: qsTr("QUANDO"); color: area.textMuted; font.bold: true; font.pixelSize: UiScale.px(11) }
                    ComboBox {
                        Layout.fillWidth: true
                        model: area.controller.automationTriggerTypes
                        currentIndex: Math.max(0, model.indexOf(area.draft.triggerType || ""))
                        onActivated: {
                            area.updateDraft("triggerType", currentText)
                            area.updateDraft("triggerParameters",
                                             currentText === "time.local"
                                             ? {"time": "09:00",
                                                "daysOfWeek": [1, 2, 3, 4, 5, 6, 7]}
                                             : {})
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: area.draft.triggerType === "time.local"
                        RowLayout {
                            Label { text: qsTr("Horário local"); color: area.textMuted }
                            TextField {
                                id: localTimeField
                                Layout.preferredWidth: 100
                                text: (area.draft.triggerParameters || ({})).time || "09:00"
                                placeholderText: qsTr("HH:mm")
                                validator: RegularExpressionValidator {
                                    regularExpression: /^([01][0-9]|2[0-3]):[0-5][0-9]$/
                                }
                                onEditingFinished: if (acceptableInput)
                                    area.updateTriggerParameter("time", text)
                            }
                        }
                        RowLayout {
                            Label { text: qsTr("Dias"); color: area.textMuted }
                            Repeater {
                                model: [{"day": 1, "label": qsTr("Seg")},
                                        {"day": 2, "label": qsTr("Ter")},
                                        {"day": 3, "label": qsTr("Qua")},
                                        {"day": 4, "label": qsTr("Qui")},
                                        {"day": 5, "label": qsTr("Sex")},
                                        {"day": 6, "label": qsTr("Sáb")},
                                        {"day": 7, "label": qsTr("Dom")}]
                                delegate: CheckBox {
                                    id: dayCheck
                                    required property var modelData
                                    text: dayCheck.modelData.label
                                    checked: ((area.draft.triggerParameters || ({})).daysOfWeek
                                              || []).indexOf(dayCheck.modelData.day) >= 0
                                    Accessible.name: qsTr("Dia %1").arg(dayCheck.modelData.day)
                                    onClicked: area.toggleTriggerDay(dayCheck.modelData.day, checked)
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("SE"); color: area.textMuted; font.bold: true; font.pixelSize: UiScale.px(11) }
                        ComboBox {
                            Layout.preferredWidth: 150
                            model: [{"id": "all", "name": qsTr("todas as condições")},
                                    {"id": "any", "name": qsTr("qualquer condição")}]
                            textRole: "name"
                            valueRole: "id"
                            currentIndex: (area.draft.conditionGroup || "all") === "any" ? 1 : 0
                            onActivated: area.updateDraft("conditionGroup", currentValue)
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: qsTr("+ condição")
                            onClicked: area.appendTo("conditions",
                                                     {"field": "event.title",
                                                      "operation": "contains",
                                                      "expected": ""})
                        }
                    }

                    Repeater {
                        model: area.draft.conditions || []
                        delegate: RowLayout {
                            id: conditionRow
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            spacing: 6
                            TextField {
                                Layout.fillWidth: true
                                text: conditionRow.modelData.field || ""
                                placeholderText: qsTr("campo (event.x ou state.y)")
                                onEditingFinished: area.updateList("conditions",
                                                                   conditionRow.index,
                                                                   {"field": text})
                            }
                            ComboBox {
                                Layout.preferredWidth: 150
                                model: area.controller.automationConditionOperations
                                currentIndex: Math.max(0, model.indexOf(
                                    conditionRow.modelData.operation || ""))
                                onActivated: area.updateList("conditions", conditionRow.index,
                                                             {"operation": currentText})
                            }
                            TextField {
                                Layout.preferredWidth: 150
                                text: conditionRow.modelData.expected === undefined
                                      ? "" : String(conditionRow.modelData.expected)
                                placeholderText: qsTr("valor")
                                onEditingFinished: area.updateList("conditions",
                                                                   conditionRow.index,
                                                                   {"expected": text})
                            }
                            Button {
                                text: "−"
                                Accessible.name: qsTr("Remover condição")
                                onClicked: area.removeFrom("conditions", conditionRow.index)
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("ENTÃO"); color: area.textMuted; font.bold: true; font.pixelSize: UiScale.px(11) }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: qsTr("+ ação")
                            onClicked: area.appendTo("actions",
                                                     {"type": "command", "parameters": {}})
                        }
                    }

                    Repeater {
                        model: area.draft.actions || []
                        delegate: ColumnLayout {
                            id: actionRow
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            spacing: 2
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6
                                Label {
                                    text: (actionRow.index + 1) + "."
                                    color: area.textMuted
                                    font.pixelSize: UiScale.px(11)
                                }
                                ComboBox {
                                    Layout.preferredWidth: 130
                                    model: area.controller.automationActionTypes
                                    currentIndex: Math.max(0, model.indexOf(
                                        actionRow.modelData.type || "command"))
                                    onActivated: area.updateList("actions", actionRow.index,
                                                                 {"type": currentText,
                                                                  "parameters": {}})
                                }
                                TextField {
                                    Layout.fillWidth: true
                                    text: {
                                        const parameters = actionRow.modelData.parameters || ({})
                                        if (actionRow.modelData.type === "integration")
                                            return parameters.integrationId || ""
                                        if (actionRow.modelData.type === "process")
                                            return parameters.executable || ""
                                        if (actionRow.modelData.type === "wait")
                                            return parameters.milliseconds === undefined
                                                   ? "" : String(parameters.milliseconds)
                                        return parameters.type || ""
                                    }
                                    placeholderText: actionRow.modelData.type === "integration"
                                                     ? qsTr("id da integração")
                                                     : actionRow.modelData.type === "process"
                                                       ? qsTr("caminho autorizado")
                                                       : actionRow.modelData.type === "wait"
                                                         ? qsTr("milissegundos") : qsTr("comando")
                                    onEditingFinished: {
                                        const parameters = Object.assign(
                                            {}, actionRow.modelData.parameters || ({}))
                                        if (actionRow.modelData.type === "integration")
                                            parameters.integrationId = text
                                        else if (actionRow.modelData.type === "process")
                                            parameters.executable = text
                                        else if (actionRow.modelData.type === "wait")
                                            parameters.milliseconds = Number(text)
                                        else
                                            parameters.type = text
                                        area.updateList("actions", actionRow.index,
                                                        {"parameters": parameters})
                                    }
                                }
                                TextField {
                                    Layout.preferredWidth: 150
                                    visible: actionRow.modelData.type === "integration"
                                    text: (actionRow.modelData.parameters || ({})).operation || ""
                                    placeholderText: qsTr("operação")
                                    onEditingFinished: {
                                        const parameters = Object.assign(
                                            {}, actionRow.modelData.parameters || ({}))
                                        parameters.operation = text
                                        area.updateList("actions", actionRow.index,
                                                        {"parameters": parameters})
                                    }
                                }
                                Button {
                                    text: "↑"
                                    Accessible.name: qsTr("Subir ação")
                                    onClicked: area.moveAction(actionRow.index, -1)
                                }
                                Button {
                                    text: "↓"
                                    Accessible.name: qsTr("Descer ação")
                                    onClicked: area.moveAction(actionRow.index, 1)
                                }
                                Button {
                                    text: "−"
                                    Accessible.name: qsTr("Remover ação")
                                    onClicked: area.removeFrom("actions", actionRow.index)
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Debounce (ms)"); color: area.textMuted; font.pixelSize: UiScale.px(11) }
                        SpinBox {
                            from: 0
                            to: 60000
                            stepSize: 250
                            editable: true
                            value: area.draft.debounceMs || 0
                            onValueModified: area.updateDraft("debounceMs", value)
                        }
                        Label { text: qsTr("Orçamento (ms)"); color: area.textMuted; font.pixelSize: UiScale.px(11) }
                        SpinBox {
                            from: 1000
                            to: 120000
                            stepSize: 1000
                            editable: true
                            value: area.draft.budgetMs || 15000
                            onValueModified: area.updateDraft("budgetMs", value)
                        }
                        Label { text: qsTr("Falhas até pausar"); color: area.textMuted; font.pixelSize: UiScale.px(11) }
                        SpinBox {
                            from: 0
                            to: 20
                            value: area.draft.failureLimit === undefined ? 5 : area.draft.failureLimit
                            onValueModified: area.updateDraft("failureLimit", value)
                        }
                        Item { Layout.fillWidth: true }
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: area.validationErrors.length > 0
                        text: area.validationErrors.join("\n")
                        color: "#ff9d9d"
                        font.pixelSize: UiScale.px(11)
                        wrapMode: Text.WordWrap
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Button { text: qsTr("Salvar"); highlighted: true; onClicked: area.save() }
                        Button {
                            text: qsTr("Ensaiar")
                            enabled: area.selectedId.length > 0
                            onClicked: area.controller.dryRunAutomation(area.selectedId, {})
                        }
                        Button {
                            text: qsTr("Retomar")
                            visible: area.selectedId.length > 0 && !(area.draft.enabled === true)
                            onClicked: {
                                area.controller.resumeAutomation(area.selectedId)
                                area.selectAutomation(area.selectedId)
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: qsTr("Excluir")
                            enabled: area.selectedId.length > 0
                            onClicked: removeConfirmation.open()
                        }
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: area.controller.automationStatus
            color: area.textMain
            font.pixelSize: UiScale.px(12)
            elide: Text.ElideRight
        }

        Label { text: qsTr("HISTÓRICO"); color: area.textMuted; font.bold: true; font.pixelSize: UiScale.px(11) }

        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            clip: true
            spacing: 2
            model: area.controller.automationRuns
            delegate: RowLayout {
                id: runRow
                required property var modelData
                width: ListView.view.width
                spacing: 10
                Label {
                    text: runRow.modelData.startedAt
                    color: area.textMuted
                    font.pixelSize: UiScale.px(11)
                    Layout.preferredWidth: 100
                }
                Label {
                    text: runRow.modelData.status
                    color: runRow.modelData.status === "completed" ? "#70e1a7"
                           : runRow.modelData.status === "dry-run" ? "#9fb0c7" : "#ff9d9d"
                    font.pixelSize: UiScale.px(11)
                    Layout.preferredWidth: 90
                }
                Label {
                    Layout.fillWidth: true
                    text: runRow.modelData.reason.length > 0
                          ? runRow.modelData.reason
                          : qsTr("%1 ações").arg(runRow.modelData.outcomes.length)
                    color: area.textMain
                    font.pixelSize: UiScale.px(11)
                    elide: Text.ElideRight
                }
                Label {
                    text: qsTr("%1 ms").arg(runRow.modelData.durationMs)
                    color: area.textMuted
                    font.pixelSize: UiScale.px(11)
                }
            }
        }
    }

    Dialog {
        id: removeConfirmation
        title: qsTr("Excluir automação")
        modal: true
        width: 380
        anchors.centerIn: parent
        standardButtons: Dialog.Cancel | Dialog.Yes
        contentItem: Label {
            width: removeConfirmation.availableWidth
            text: qsTr("A automação e o histórico dela serão apagados. Confirmar?")
            color: area.textMain
            wrapMode: Text.WordWrap
        }
        onAccepted: {
            area.controller.removeAutomation(area.selectedId)
            if (area.controller.automations.length > 0)
                area.selectAutomation(area.controller.automations[0].id)
            else
                area.newAutomation()
        }
    }

    Dialog {
        id: processDialog
        title: qsTr("Processos externos autorizados")
        modal: true
        width: Math.min(620, area.width - 60)
        anchors.centerIn: parent
        standardButtons: Dialog.Close

        contentItem: ColumnLayout {
            spacing: 8
            Switch {
                text: qsTr("Permitir que automações executem processos externos")
                checked: area.controller.processActionsEnabled
                palette.windowText: area.textMain
                onClicked: area.controller.processActionsEnabled = checked
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Só executáveis desta lista podem ser acionados, sempre pelo caminho real e sem shell.")
                color: area.textMuted
                font.pixelSize: UiScale.px(11)
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: executablePath
                    Layout.fillWidth: true
                    placeholderText: qsTr("/caminho/absoluto/do/executavel")
                }
                Button { text: qsTr("Procurar"); onClicked: executableDialog.open() }
                Button {
                    text: qsTr("Autorizar")
                    enabled: executablePath.text.length > 0
                    onClicked: {
                        const result = area.controller.authorizeExecutable(executablePath.text, "")
                        if (result.accepted)
                            executablePath.text = ""
                    }
                }
            }
            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: 160
                clip: true
                spacing: 2
                model: area.controller.authorizedExecutables
                delegate: RowLayout {
                    id: executableRow
                    required property var modelData
                    width: ListView.view.width
                    Label {
                        Layout.fillWidth: true
                        text: executableRow.modelData.canonicalPath
                        color: area.textMain
                        font.pixelSize: UiScale.px(11)
                        elide: Text.ElideMiddle
                    }
                    Label {
                        text: executableRow.modelData.authorizedAt
                        color: area.textMuted
                        font.pixelSize: UiScale.px(10)
                    }
                    Button {
                        text: qsTr("Revogar")
                        onClicked: area.controller.revokeExecutable(
                                       executableRow.modelData.canonicalPath)
                    }
                }
            }
        }
    }

    FileDialog {
        id: executableDialog
        title: qsTr("Escolher executável")
        onAccepted: executablePath.text = selectedFile.toString().replace("file://", "")
    }

    FileDialog {
        id: importDialog
        title: qsTr("Importar automações")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Automações HolyScreen (*.json)")]
        onAccepted: area.controller.importAutomations(selectedFile)
    }

    FileDialog {
        id: exportDialog
        title: qsTr("Exportar automações")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("Automações HolyScreen (*.json)")]
        onAccepted: area.controller.exportAutomations(selectedFile)
    }
}
