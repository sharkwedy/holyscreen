pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Área de integrações do operador. A tela apenas edita e dispara comandos:
// validação, segredos e execução vivem no domínio.
Dialog {
    id: area

    required property var controller

    title: "Integrações"
    modal: true
    width: Math.min(940, parent ? parent.width - 60 : 940)
    height: Math.min(720, parent ? parent.height - 60 : 720)
    anchors.centerIn: parent
    standardButtons: Dialog.Close

    readonly property color panel: "#20252a"
    readonly property color panelHigh: "#2a3036"
    readonly property color line: "#46505a"
    readonly property color textMain: "#f2f4f5"
    readonly property color textMuted: "#aab2b8"

    property string selectedId: ""
    //! Cópia editável da definição selecionada.
    property var draft: ({})

    readonly property var fieldsByType: ({
        "http": [{"key": "url", "label": "URL", "hint": "https://servidor.local/hook"},
                 {"key": "method", "label": "Método", "options": ["POST", "GET", "PUT", "PATCH", "DELETE"]},
                 {"key": "body", "label": "Corpo", "multiline": true,
                  "hint": "{\"slide\":\"{{slide}}\"}"}],
        "websocket": [{"key": "url", "label": "URL", "hint": "ws://127.0.0.1:9000"},
                      {"key": "message", "label": "Mensagem", "multiline": true}],
        "obs": [{"key": "host", "label": "Host", "hint": "127.0.0.1"},
                {"key": "port", "label": "Porta", "number": true, "fallback": 4455}],
        "midi": [{"key": "port", "label": "Porta de saída", "midiPorts": true},
                 {"key": "channel", "label": "Canal", "number": true, "fallback": 1}],
        "osc": [{"key": "host", "label": "Host", "hint": "127.0.0.1"},
                {"key": "port", "label": "Porta", "number": true, "fallback": 9000},
                {"key": "address", "label": "Caminho OSC", "hint": "/cena/1"}]
    })

    readonly property var secretFieldByType: ({
        "http": "headers.Authorization",
        "obs": "passwordReference"
    })

    function fields() {
        return area.fieldsByType[area.draft.type] || []
    }

    function secretField() {
        return area.secretFieldByType[area.draft.type] || ""
    }

    function configurationValue(key, fallback) {
        const configuration = area.draft.configuration || ({})
        const value = configuration[key]
        return value === undefined || value === "" ? fallback : value
    }

    function updateConfiguration(key, value) {
        const configuration = Object.assign({}, area.draft.configuration || ({}))
        configuration[key] = value
        area.draft = Object.assign({}, area.draft, {"configuration": configuration})
    }

    function updateDraft(key, value) {
        const changes = {}
        changes[key] = value
        area.draft = Object.assign({}, area.draft, changes)
    }

    function selectIntegration(id) {
        const definition = area.controller.integrationDefinition(id)
        area.selectedId = definition.id || ""
        area.draft = definition
    }

    function startNewIntegration() {
        area.selectedId = ""
        area.draft = {"id": "", "name": "Nova integração", "type": "http", "enabled": true,
                      "timeoutMs": 5000, "retryAttempts": 1, "retryBackoffMs": 250,
                      "configuration": {}, "secretReferences": []}
    }

    function save() {
        const result = area.controller.saveIntegration(area.draft)
        area.validationErrors = result.errors || []
        if (result.accepted)
            area.selectIntegration(result.id)
    }

    property var validationErrors: []

    onOpened: {
        if (area.controller.integrations.length > 0)
            area.selectIntegration(area.controller.integrations[0].id)
        else
            area.startNewIntegration()
    }

    background: Rectangle {
        color: area.panel
        border.color: area.line
        radius: 8
    }

    contentItem: ColumnLayout {
        spacing: 10

        Label {
            Layout.fillWidth: true
            visible: !area.controller.integrationSecretsPersistent
            text: "Sem cofre do sistema (" + area.controller.integrationSecretBackend
                  + "): os segredos valem apenas nesta sessão."
            color: "#ffba70"
            font.pixelSize: 11
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // Lista de integrações.
            ColumnLayout {
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholderText: "Pesquisar"
                    }
                    Button {
                        text: "+"
                        Accessible.name: "Nova integração"
                        onClicked: area.startNewIntegration()
                    }
                }

                ListView {
                    id: integrationList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 4
                    model: area.controller.integrations.filter(function (item) {
                        const term = searchField.text.toLowerCase()
                        return term.length === 0 || item.name.toLowerCase().indexOf(term) >= 0
                               || item.type.indexOf(term) >= 0
                    })
                    delegate: Rectangle {
                        id: integrationRow
                        required property var modelData
                        width: ListView.view.width
                        height: 52
                        radius: 6
                        color: integrationRow.modelData.id === area.selectedId ? "#2f3a4b"
                                                                               : area.panelHigh
                        border.color: integrationRow.modelData.id === area.selectedId
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
                                    text: integrationRow.modelData.name
                                    color: area.textMain
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                                Label {
                                    text: integrationRow.modelData.type.toUpperCase()
                                    color: area.textMuted
                                    font.pixelSize: 10
                                }
                            }
                            Switch {
                                checked: integrationRow.modelData.enabled
                                Accessible.name: "Ativar integração"
                                onClicked: area.controller.setIntegrationEnabled(
                                               integrationRow.modelData.id, checked)
                            }
                        }
                        TapHandler {
                            onTapped: area.selectIntegration(integrationRow.modelData.id)
                        }
                    }
                    Label {
                        anchors.centerIn: parent
                        visible: integrationList.count === 0
                        text: "Nenhuma integração configurada"
                        color: area.textMuted
                        font.pixelSize: 12
                    }
                }
            }

            // Editor da integração selecionada.
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
                            placeholderText: "Nome"
                            onEditingFinished: area.updateDraft("name", text)
                        }
                        ComboBox {
                            Layout.preferredWidth: 140
                            model: area.controller.integrationTypes
                            currentIndex: Math.max(0, model.indexOf(area.draft.type || "http"))
                            onActivated: area.updateDraft("type", currentText)
                        }
                    }

                    Repeater {
                        model: area.fields()
                        delegate: ColumnLayout {
                            id: fieldRow
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 2
                            Label {
                                text: fieldRow.modelData.label
                                color: area.textMuted
                                font.pixelSize: 11
                            }
                            ComboBox {
                                Layout.fillWidth: true
                                visible: fieldRow.modelData.options !== undefined
                                         || fieldRow.modelData.midiPorts === true
                                model: fieldRow.modelData.midiPorts === true
                                       ? area.controller.midiOutputPorts()
                                       : (fieldRow.modelData.options || [])
                                editable: fieldRow.modelData.midiPorts === true
                                currentIndex: Math.max(0, model.indexOf(
                                    area.configurationValue(fieldRow.modelData.key, "")))
                                onActivated: area.updateConfiguration(fieldRow.modelData.key,
                                                                      currentText)
                            }
                            SpinBox {
                                Layout.fillWidth: true
                                visible: fieldRow.modelData.number === true
                                from: 0
                                to: 65535
                                editable: true
                                value: area.configurationValue(fieldRow.modelData.key,
                                                               fieldRow.modelData.fallback || 0)
                                onValueModified: area.updateConfiguration(fieldRow.modelData.key,
                                                                          value)
                            }
                            TextArea {
                                Layout.fillWidth: true
                                visible: fieldRow.modelData.multiline === true
                                text: area.configurationValue(fieldRow.modelData.key, "")
                                placeholderText: fieldRow.modelData.hint || ""
                                onEditingFinished: area.updateConfiguration(
                                                       fieldRow.modelData.key, text)
                            }
                            TextField {
                                Layout.fillWidth: true
                                visible: fieldRow.modelData.options === undefined
                                         && fieldRow.modelData.number !== true
                                         && fieldRow.modelData.multiline !== true
                                         && fieldRow.modelData.midiPorts !== true
                                text: area.configurationValue(fieldRow.modelData.key, "")
                                placeholderText: fieldRow.modelData.hint || ""
                                onEditingFinished: area.updateConfiguration(
                                                       fieldRow.modelData.key, text)
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "Timeout (ms)"; color: area.textMuted; font.pixelSize: 11 }
                        SpinBox {
                            from: 250
                            to: 60000
                            stepSize: 250
                            editable: true
                            value: area.draft.timeoutMs || 5000
                            onValueModified: area.updateDraft("timeoutMs", value)
                        }
                        Label { text: "Tentativas"; color: area.textMuted; font.pixelSize: 11 }
                        SpinBox {
                            from: 1
                            to: 5
                            value: area.draft.retryAttempts || 1
                            onValueModified: area.updateDraft("retryAttempts", value)
                        }
                        Item { Layout.fillWidth: true }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        visible: area.secretField().length > 0 && area.selectedId.length > 0
                        TextField {
                            id: secretField
                            Layout.fillWidth: true
                            echoMode: TextInput.Password
                            placeholderText: "Segredo (fica no cofre do sistema)"
                        }
                        Button {
                            text: "Guardar segredo"
                            enabled: secretField.text.length > 0
                            onClicked: {
                                area.controller.setIntegrationSecret(area.selectedId,
                                                                     area.secretField(),
                                                                     secretField.text)
                                secretField.text = ""
                                area.selectIntegration(area.selectedId)
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: area.validationErrors.length > 0
                        text: area.validationErrors.join("\n")
                        color: "#ff9d9d"
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Button { text: "Salvar"; highlighted: true; onClicked: area.save() }
                        Button {
                            text: "Testar conexão"
                            enabled: area.selectedId.length > 0
                            onClicked: area.controller.testIntegration(area.selectedId)
                        }
                        Button {
                            text: "Duplicar"
                            enabled: area.selectedId.length > 0
                            onClicked: {
                                const copyId = area.controller.duplicateIntegration(area.selectedId)
                                if (copyId.length > 0)
                                    area.selectIntegration(copyId)
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "Excluir"
                            enabled: area.selectedId.length > 0
                            onClicked: removeConfirmation.open()
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        visible: area.selectedId.length > 0
                        spacing: 8
                        ComboBox {
                            id: operationPicker
                            Layout.preferredWidth: 220
                            model: {
                                const selected = area.controller.integrations.filter(
                                    function (item) { return item.id === area.selectedId })
                                return selected.length > 0 ? selected[0].operations : []
                            }
                        }
                        Button {
                            text: "Executar operação"
                            enabled: operationPicker.currentText.length > 0
                            onClicked: area.controller.executeIntegration(
                                           area.selectedId, operationPicker.currentText, {})
                        }
                        Item { Layout.fillWidth: true }
                    }
                }
            }
        }

        // Estado não modal da última chamada.
        Label {
            Layout.fillWidth: true
            text: area.controller.integrationStatus
            color: area.textMain
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        Label { text: "HISTÓRICO"; color: area.textMuted; font.bold: true; font.pixelSize: 11 }

        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 130
            clip: true
            spacing: 2
            model: area.controller.integrationHistory
            delegate: Rectangle {
                id: historyRow
                required property var modelData
                width: ListView.view.width
                height: 26
                color: "transparent"
                RowLayout {
                    anchors.fill: parent
                    spacing: 10
                    Label {
                        text: historyRow.modelData.occurredAt
                        color: area.textMuted
                        font.pixelSize: 11
                        Layout.preferredWidth: 100
                    }
                    Label {
                        text: historyRow.modelData.operation
                        color: area.textMain
                        font.pixelSize: 11
                        Layout.preferredWidth: 150
                        elide: Text.ElideRight
                    }
                    Label {
                        Layout.fillWidth: true
                        text: historyRow.modelData.accepted
                              ? historyRow.modelData.message
                              : (historyRow.modelData.errorCode + " — "
                                 + historyRow.modelData.message)
                        color: historyRow.modelData.accepted ? "#70e1a7" : "#ff9d9d"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                    Label {
                        text: historyRow.modelData.durationMs + " ms"
                        color: area.textMuted
                        font.pixelSize: 11
                    }
                }
            }
        }
    }

    Dialog {
        id: removeConfirmation
        title: "Excluir integração"
        modal: true
        width: 380
        anchors.centerIn: parent
        standardButtons: Dialog.Cancel | Dialog.Yes
        contentItem: Label {
            width: removeConfirmation.availableWidth
            text: "A integração e o histórico dela serão apagados. Confirmar?"
            color: area.textMain
            wrapMode: Text.WordWrap
        }
        onAccepted: {
            area.controller.removeIntegration(area.selectedId)
            if (area.controller.integrations.length > 0)
                area.selectIntegration(area.controller.integrations[0].id)
            else
                area.startNewIntegration()
        }
    }
}
