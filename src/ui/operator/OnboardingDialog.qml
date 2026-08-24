pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: onboarding
    required property var controller
    signal openSettings(int tabIndex)
    signal openBible()

    title: qsTr("Configuração guiada do HolyScreen")
    modal: true
    width: Math.min(760, parent ? parent.width - 80 : 760)
    height: Math.min(620, parent ? parent.height - 80 : 620)
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    property int currentStep: 0
    readonly property var steps: [
        {"id":"screens", "title":qsTr("Telas"), "description":qsTr("Escolha as saídas de público, palco e transmissão."), "tab":1},
        {"id":"audio", "title":qsTr("Áudio"), "description":qsTr("Ajuste volume, repetição e comportamento de imagens."), "tab":2},
        {"id":"library", "title":qsTr("Biblioteca"), "description":qsTr("Importe ao menos uma pasta de mídia."), "tab":0},
        {"id":"bible", "title":qsTr("Bíblia"), "description":qsTr("Importe e selecione uma tradução bíblica."), "tab":-1},
        {"id":"remote", "title":qsTr("Controle remoto"), "description":qsTr("Defina uma senha antes de habilitar o servidor local."), "tab":4},
        {"id":"broadcast", "title":qsTr("Broadcast"), "description":qsTr("Configure uma saída para transmissão ou deixe esta etapa para depois."), "tab":1}
    ]

    function stepComplete(stepId) {
        if (stepId === "screens") return controller.outputContext.outputWindows.length > 0
        if (stepId === "audio") return controller.mediaContext.audioOutputConfigured
        if (stepId === "library") return controller.mediaContext.mediaFolders.length > 0
        if (stepId === "bible") return controller.bibleContext.bibleTranslations.length > 0
        if (stepId === "remote") return controller.remotePasswordConfigured
        if (stepId === "broadcast") {
            for (let index = 0; index < controller.outputContext.screens.length; ++index) {
                if (controller.outputContext.screens[index].role === "broadcast") return true
            }
        }
        return false
    }

    function stepSkipped(stepId) {
        return controller.onboardingSkippedSteps.indexOf(stepId) >= 0
    }

    function firstIncompleteStep() {
        for (let index = 0; index < steps.length; ++index) {
            if (!stepComplete(steps[index].id) && !stepSkipped(steps[index].id)) return index
        }
        for (let index = 0; index < steps.length; ++index) {
            if (!stepComplete(steps[index].id)) return index
        }
        return steps.length - 1
    }

    function configureCurrentStep() {
        const step = steps[currentStep]
        controller.resumeOnboardingStep(step.id)
        if (step.id === "bible") openBible()
        else openSettings(step.tab)
    }

    onOpened: currentStep = firstIncompleteStep()

    background: Rectangle {
        color: "#20252a"
        border.color: "#46505a"
        radius: 10
    }

    contentItem: ColumnLayout {
        spacing: 16
        Label {
            Layout.fillWidth: true
            text: qsTr("Prepare o operador sem interromper a configuração. Etapas pendentes podem ser retomadas depois em Configurações.")
            color: "#aab2b8"
            wrapMode: Text.WordWrap
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ListView {
                id: checklist
                Layout.preferredWidth: 230
                Layout.fillHeight: true
                spacing: 6
                clip: true
                model: onboarding.steps
                delegate: ItemDelegate {
                    required property var modelData
                    required property int index
                    width: checklist.width
                    highlighted: onboarding.currentStep === index
                    text: (onboarding.stepComplete(modelData.id) ? "✓  "
                           : onboarding.stepSkipped(modelData.id) ? "–  " : "○  ")
                          + modelData.title
                    Accessible.name: text
                    onClicked: onboarding.currentStep = index
                }
            }
            Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#46505a" }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 14
                Label {
                    text: onboarding.steps[onboarding.currentStep].title
                    color: "#f2f4f5"
                    font.bold: true
                    font.pixelSize: UiScale.px(24)
                }
                Label {
                    Layout.fillWidth: true
                    text: onboarding.steps[onboarding.currentStep].description
                    color: "#aab2b8"
                    wrapMode: Text.WordWrap
                }
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 54
                    radius: 7
                    color: onboarding.stepComplete(onboarding.steps[onboarding.currentStep].id)
                           ? "#183f31"
                           : onboarding.stepSkipped(onboarding.steps[onboarding.currentStep].id)
                             ? "#283746" : "#3c3421"
                    Label {
                        anchors.centerIn: parent
                        text: onboarding.stepComplete(onboarding.steps[onboarding.currentStep].id)
                              ? qsTr("Etapa configurada")
                              : onboarding.stepSkipped(onboarding.steps[onboarding.currentStep].id)
                                ? qsTr("Etapa adiada") : qsTr("Configuração pendente")
                        color: onboarding.stepComplete(onboarding.steps[onboarding.currentStep].id)
                               ? "#70e1a7"
                               : onboarding.stepSkipped(onboarding.steps[onboarding.currentStep].id)
                                 ? "#9fc5e8" : "#f0c36a"
                        font.bold: true
                    }
                }
                RowLayout {
                    Button {
                        text: onboarding.stepSkipped(onboarding.steps[onboarding.currentStep].id)
                              ? qsTr("Retomar esta etapa") : qsTr("Configurar esta etapa")
                        Accessible.name: text
                        onClicked: onboarding.configureCurrentStep()
                    }
                    Button {
                        visible: !onboarding.stepComplete(onboarding.steps[onboarding.currentStep].id)
                                 && !onboarding.stepSkipped(onboarding.steps[onboarding.currentStep].id)
                        text: qsTr("Fazer depois")
                        flat: true
                        Accessible.name: text
                        onClicked: {
                            onboarding.controller.skipOnboardingStep(
                                onboarding.steps[onboarding.currentStep].id)
                            onboarding.currentStep = onboarding.firstIncompleteStep()
                        }
                    }
                }
                Item { Layout.fillHeight: true }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("As mensagens deste assistente são informativas e não ativam automaticamente telas, rede ou Broadcast.")
                    color: "#8d979f"
                    wrapMode: Text.WordWrap
                    font.pixelSize: UiScale.px(11)
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Button {
                text: qsTr("Voltar")
                enabled: onboarding.currentStep > 0
                onClicked: --onboarding.currentStep
            }
            Button {
                text: qsTr("Próxima")
                enabled: onboarding.currentStep < onboarding.steps.length - 1
                onClicked: ++onboarding.currentStep
            }
            Item { Layout.fillWidth: true }
            Button { text: qsTr("Agora não"); flat: true; onClicked: onboarding.close() }
            Button {
                text: qsTr("Concluir configuração")
                highlighted: true
                onClicked: {
                    onboarding.controller.completeOnboarding()
                    onboarding.close()
                }
            }
        }
    }

    Connections {
        target: onboarding.controller
        function onOnboardingChanged() {
            if (!onboarding.controller.onboardingCompleted && !onboarding.visible)
                onboarding.open()
        }
    }

    Component.onCompleted: {
        if (!controller.onboardingCompleted) {
            Qt.callLater(function() { onboarding.open() })
        }
    }
}
