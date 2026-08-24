pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Dialog {
    id: settings
    required property var controller
    signal openLibrary()
    signal chooseBackground()
    signal restoreLayout()
    property string profileStatus: ""

    function openTab(index) {
        settingsTabs.currentIndex = Math.max(0, Math.min(settingsTabs.count - 1, index))
        open()
    }

    title: qsTr("Configurações")
    modal: true
    width: Math.min(860, parent ? parent.width - 80 : 860)
    height: Math.min(680, parent ? parent.height - 80 : 680)
    anchors.centerIn: parent
    standardButtons: Dialog.Close

    readonly property color panel: "#20252a"
    readonly property color panelHigh: "#2a3036"
    readonly property color line: "#46505a"
    readonly property color textMain: "#f2f4f5"
    readonly property color textMuted: "#aab2b8"

    function valueIndex(model, value) {
        for (let index = 0; index < model.length; ++index) {
            if (model[index].id === value)
                return index
        }
        return 0
    }

    background: Rectangle {
        color: settings.panel
        border.color: settings.line
        radius: 8
    }

    contentItem: ColumnLayout {
        spacing: 12

        TabBar {
            id: settingsTabs
            Layout.fillWidth: true
            TabButton { text: qsTr("Geral"); palette.buttonText: settings.textMain }
            TabButton { text: qsTr("Telas"); palette.buttonText: settings.textMain }
            TabButton { text: qsTr("Mídia"); palette.buttonText: settings.textMain }
            TabButton { text: qsTr("Aparência"); palette.buttonText: settings.textMain }
            TabButton { text: qsTr("Remoto"); palette.buttonText: settings.textMain }
            TabButton { text: qsTr("Avançado"); palette.buttonText: settings.textMain }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: settingsTabs.currentIndex

            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 14
                    Label { text: qsTr("Biblioteca"); color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Gerencie as pastas usadas pelo catálogo de áudio, vídeo e imagens.")
                        color: settings.textMuted
                        wrapMode: Text.WordWrap
                    }
                    RowLayout {
                        Button { text: qsTr("Abrir biblioteca"); onClicked: settings.openLibrary() }
                        Button { text: qsTr("Reexaminar pastas"); onClicked: settings.controller.mediaContext.rescanMediaFolders() }
                        Button { text: qsTr("Restaurar layout"); onClicked: settings.restoreLayout() }
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label { text: qsTr("Comunicação com o palco"); color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    TextArea {
                        id: stageMessageSetting
                        Layout.fillWidth: true
                        Layout.preferredHeight: 90
                        text: settings.controller.stageMessage
                        placeholderText: qsTr("Mensagem para as telas configuradas como palco")
                        wrapMode: TextEdit.Wrap
                    }
                    Button {
                        text: qsTr("Salvar mensagem")
                        onClicked: settings.controller.stageMessage = stageMessageSetting.text
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label { text: qsTr("Atualizações e manutenção"); color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    TextField {
                        Layout.fillWidth: true
                        text: settings.controller.maintenanceContext.updateEndpoint
                        placeholderText: qsTr("API oficial de Releases do GitHub")
                        readOnly: true
                    }
                    RowLayout {
                        Button { text: qsTr("Verificar atualizações"); onClicked: settings.controller.maintenanceContext.checkForUpdates() }
                        Button { text: qsTr("Criar backup"); onClicked: settings.controller.maintenanceContext.createBackup() }
                    }
                    Label {
                        Layout.fillWidth: true
                        text: settings.controller.maintenanceContext.updateStatus.length > 0
                              ? settings.controller.maintenanceContext.updateStatus
                              : settings.controller.maintenanceContext.autosaveStatus
                        color: settings.textMuted
                        wrapMode: Text.WordWrap
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label { text: qsTr("Perfil do operador"); color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Importe ou exporte telas, aparência, mídia, biblioteca e preferências. Senhas, tokens e credenciais nunca são incluídos.")
                        color: settings.textMuted
                        wrapMode: Text.WordWrap
                    }
                    RowLayout {
                        Button { text: qsTr("Importar perfil"); onClicked: importProfileDialog.open() }
                        Button { text: qsTr("Exportar perfil"); onClicked: exportProfileDialog.open() }
                        Button {
                            text: qsTr("Configuração guiada")
                            onClicked: settings.controller.reopenOnboarding()
                        }
                    }
                    RowLayout {
                        Label { text: qsTr("Idioma"); color: settings.textMain; Layout.preferredWidth: 120 }
                        ComboBox {
                            model: [{"text":qsTr("Português (Brasil)"), "value":"pt-BR"},
                                    {"text":qsTr("English (United States)"), "value":"en-US"}]
                            textRole: "text"
                            valueRole: "value"
                            currentIndex: settings.controller.locale === "en-US" ? 1 : 0
                            onActivated: settings.controller.locale = currentValue
                        }
                        CheckBox {
                            text: qsTr("Modo demonstração")
                            checked: settings.controller.demoMode
                            palette.windowText: settings.textMain
                            onClicked: settings.controller.demoMode = checked
                        }
                    }
                    Label {
                        visible: settings.controller.localeRestartRequired
                        Layout.fillWidth: true
                        text: qsTr("Reinicie o HolyScreen para aplicar o novo idioma.")
                        color: "#f0c36a"
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        visible: settings.profileStatus.length > 0
                        Layout.fillWidth: true
                        text: settings.profileStatus
                        color: settings.textMuted
                        wrapMode: Text.WordWrap
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label { text: qsTr("Atalhos de teclado"); color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    Repeater {
                        model: [{"id":"blackout", "label":qsTr("Blackout")},
                                {"id":"next", "label":qsTr("Próximo slide")},
                                {"id":"previous", "label":qsTr("Slide anterior")},
                                {"id":"stop", "label":qsTr("Parar apresentação")},
                                {"id":"quickBible", "label":qsTr("Busca bíblica")}]
                        delegate: RowLayout {
                            id: shortcutRow
                            required property var modelData
                            Layout.fillWidth: true
                            Label {
                                text: shortcutRow.modelData.label
                                color: settings.textMain
                                Layout.preferredWidth: 180
                            }
                            TextField {
                                Layout.fillWidth: true
                                text: settings.controller.shortcuts[shortcutRow.modelData.id] || ""
                                Accessible.name: qsTr("Atalho para %1").arg(shortcutRow.modelData.label)
                                onEditingFinished: {
                                    if (!settings.controller.setShortcut(shortcutRow.modelData.id, text)) {
                                        settings.profileStatus = qsTr("Atalho inválido ou já utilizado.")
                                        text = settings.controller.shortcuts[shortcutRow.modelData.id] || ""
                                    } else {
                                        settings.profileStatus = qsTr("Atalho atualizado.")
                                    }
                                }
                            }
                        }
                    }
                }
            }

            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 10
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Monitores detectados"); color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                        Item { Layout.fillWidth: true }
                        Button { text: qsTr("Identificar"); onClicked: settings.controller.outputContext.identifyScreens() }
                        Button { text: qsTr("Ativar todas"); onClicked: settings.controller.outputContext.enableAllScreens() }
                    }
                    Repeater {
                        model: settings.controller.outputContext.screens
                        delegate: Rectangle {
                            id: screenRow
                            required property var modelData
                            Layout.fillWidth: true
                            implicitHeight: screenRow.modelData.primary ? 64
                                            : screenRow.modelData.selected
                                              && screenRow.modelData.role === "broadcast" ? 430
                                            : 116
                            radius: 6
                            color: settings.panelHigh
                            border.color: screenRow.modelData.selected ? "#7294ff" : settings.line
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                RowLayout {
                                    Layout.fillWidth: true
                                    CheckBox {
                                        checked: screenRow.modelData.primary || screenRow.modelData.selected
                                        enabled: !screenRow.modelData.primary
                                        onClicked: settings.controller.outputContext.toggleScreen(screenRow.modelData.id, checked)
                                    }
                                    TextField {
                                        Layout.fillWidth: true
                                        text: screenRow.modelData.name
                                        readOnly: screenRow.modelData.primary || !screenRow.modelData.selected
                                        onEditingFinished: settings.controller.outputContext.setOutputDisplayName(screenRow.modelData.id, text)
                                    }
                                    Label {
                                        text: screenRow.modelData.primary ? qsTr("OPERADOR")
                                              : screenRow.modelData.selected ? qsTr("ATIVA") : qsTr("INATIVA")
                                        color: screenRow.modelData.selected ? "#9eb5ff" : settings.textMuted
                                        font.bold: true
                                        font.pixelSize: 10
                                    }
                                }
                                RowLayout {
                                    visible: !screenRow.modelData.primary && screenRow.modelData.selected
                                    Layout.fillWidth: true
                                    CheckBox {
                                        text: qsTr("Exibir vídeo")
                                        checked: screenRow.modelData.mediaEnabled
                                        palette.windowText: settings.textMain
                                        onClicked: settings.controller.outputContext.setOutputMediaEnabled(screenRow.modelData.id, checked)
                                    }
                                    ComboBox {
                                        Layout.preferredWidth: 210
                                        model: [{"id":"audience", "name":qsTr("Saída para o público")},
                                                {"id":"stage", "name":qsTr("Saída de palco")},
                                                {"id":"broadcast", "name":qsTr("Saída de transmissão")}]
                                        textRole: "name"
                                        valueRole: "id"
                                        currentIndex: settings.valueIndex(model, screenRow.modelData.role)
                                        onActivated: settings.controller.outputContext.setOutputRole(screenRow.modelData.id, currentValue)
                                    }
                                    Item { Layout.fillWidth: true }
                                }
                                BroadcastSettings {
                                    Layout.fillWidth: true
                                    visible: screenRow.modelData.selected
                                             && screenRow.modelData.role === "broadcast"
                                    controller: settings.controller
                                    screen: screenRow.modelData
                                    textMain: settings.textMain
                                    textMuted: settings.textMuted
                                }
                            }
                        }
                    }
                }
            }

            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 14
                    Label { text: qsTr("Reprodução"); color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    RowLayout {
                        Label { text: qsTr("Volume"); color: settings.textMain; Layout.preferredWidth: 160 }
                        Slider { Layout.fillWidth: true; from: 0; to: 1; value: settings.controller.mediaContext.mediaVolume; onMoved: settings.controller.mediaContext.mediaVolume = value }
                    }
                    RowLayout {
                        Label { text: qsTr("Saída de áudio"); color: settings.textMain; Layout.preferredWidth: 160 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [{"id":"", "displayName":qsTr("Selecione um dispositivo")}]
                                   .concat(settings.controller.mediaContext.audioOutputs)
                            textRole: "displayName"
                            valueRole: "id"
                            currentIndex: settings.valueIndex(
                                              model, settings.controller.mediaContext.audioOutputId)
                            Accessible.name: qsTr("Dispositivo de saída de áudio")
                            onActivated: {
                                if (currentValue.length > 0)
                                    settings.controller.mediaContext.audioOutputId = currentValue
                            }
                        }
                    }
                    Label {
                        visible: settings.controller.mediaContext.audioOutputs.length === 0
                        Layout.fillWidth: true
                        text: qsTr("Nenhum dispositivo de áudio foi detectado. Verifique a conexão e tente novamente.")
                        color: "#f0c36a"
                        wrapMode: Text.WordWrap
                    }
                    RowLayout {
                        Label { text: qsTr("Repetição"); color: settings.textMain; Layout.preferredWidth: 160 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [{"id":"off", "name":qsTr("Sem repetição")},
                                    {"id":"one", "name":qsTr("Repetir item")},
                                    {"id":"all", "name":qsTr("Repetir playlist")}]
                            textRole: "name"; valueRole: "id"
                            currentIndex: settings.valueIndex(model, settings.controller.mediaContext.mediaRepeatMode)
                            onActivated: settings.controller.mediaContext.mediaRepeatMode = currentValue
                        }
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label { text: qsTr("Imagens"); color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    RowLayout {
                        Label { text: qsTr("Ajuste"); color: settings.textMain; Layout.preferredWidth: 160 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [{"id":"contain", "name":qsTr("Conter")},
                                    {"id":"cover", "name":qsTr("Preencher")},
                                    {"id":"stretch", "name":qsTr("Esticar")},
                                    {"id":"center", "name":qsTr("Centralizar")}]
                            textRole: "name"; valueRole: "id"
                            currentIndex: settings.valueIndex(model, settings.controller.imageFit)
                            onActivated: settings.controller.imageFit = currentValue
                        }
                    }
                    CheckBox {
                        text: qsTr("Avançar imagens automaticamente")
                        checked: settings.controller.imageAutoplay
                        palette.windowText: settings.textMain
                        onClicked: settings.controller.imageAutoplay = checked
                    }
                    RowLayout {
                        enabled: settings.controller.imageAutoplay
                        Label { text: qsTr("Intervalo (segundos)"); color: settings.textMain; Layout.preferredWidth: 160 }
                        SpinBox { from: 1; to: 3600; value: Math.round(settings.controller.imageIntervalMs / 1000); onValueModified: settings.controller.imageIntervalMs = value * 1000 }
                    }
                }
            }

            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 14
                    Label { text: qsTr("Fundo da apresentação"); color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 150
                        color: settings.controller.wallpaperColor
                        border.color: settings.line
                        radius: 6
                        clip: true
                        Image {
                            anchors.fill: parent
                            source: settings.controller.wallpaperSource
                            visible: source.toString().length > 0
                            asynchronous: true
                            fillMode: settings.controller.wallpaperFit === "contain" ? Image.PreserveAspectFit
                                      : settings.controller.wallpaperFit === "stretch" ? Image.Stretch
                                      : settings.controller.wallpaperFit === "center" ? Image.Pad
                                      : Image.PreserveAspectCrop
                        }
                        Label {
                            anchors.centerIn: parent
                            visible: settings.controller.wallpaperSource.toString().length === 0
                            text: qsTr("Fundo de cor sólida")
                            color: settings.textMain
                            style: Text.Outline
                            styleColor: "#80000000"
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: qsTr("Escolher imagem…"); onClicked: settings.chooseBackground() }
                        Button {
                            text: qsTr("Remover imagem")
                            enabled: settings.controller.wallpaperSource.toString().length > 0
                            onClicked: settings.controller.wallpaperSource = ""
                        }
                        Label {
                            Layout.fillWidth: true
                            text: settings.controller.wallpaperSource.toString().length > 0
                                  ? settings.controller.wallpaperSource.toString()
                                  : qsTr("Nenhuma imagem selecionada")
                            color: settings.textMuted
                            elide: Text.ElideMiddle
                        }
                    }
                    RowLayout {
                        Label { text: qsTr("Cor"); color: settings.textMain; Layout.preferredWidth: 160 }
                        TextField { Layout.fillWidth: true; text: settings.controller.wallpaperColor; placeholderText: "#000000"; onEditingFinished: settings.controller.wallpaperColor = text }
                    }
                    RowLayout {
                        Label { text: qsTr("Ajuste do fundo"); color: settings.textMain; Layout.preferredWidth: 160 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [{"id":"cover", "name":qsTr("Preencher")},
                                    {"id":"contain", "name":qsTr("Conter")},
                                    {"id":"stretch", "name":qsTr("Esticar")},
                                    {"id":"center", "name":qsTr("Centralizar")}]
                            textRole: "name"; valueRole: "id"
                            currentIndex: settings.valueIndex(model, settings.controller.wallpaperFit)
                            onActivated: settings.controller.wallpaperFit = currentValue
                        }
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label { text: qsTr("Relógio"); color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    CheckBox {
                        text: qsTr("Exibir relógio nas saídas")
                        checked: settings.controller.clockVisible
                        palette.windowText: settings.textMain
                        onClicked: settings.controller.clockVisible = checked
                    }
                    RowLayout {
                        Label { text: qsTr("Formato"); color: settings.textMain; Layout.preferredWidth: 160 }
                        TextField { Layout.fillWidth: true; text: settings.controller.clockFormat; onEditingFinished: settings.controller.clockFormat = text }
                    }
                    RowLayout {
                        Label { text: qsTr("Tamanho"); color: settings.textMain; Layout.preferredWidth: 160 }
                        SpinBox { from: 12; to: 300; value: settings.controller.clockFontSize; onValueModified: settings.controller.clockFontSize = value }
                    }
                    RowLayout {
                        Label { text: qsTr("Nome da fonte"); color: settings.textMain; Layout.preferredWidth: 160 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: Qt.fontFamilies()
                            currentIndex: Math.max(0, model.indexOf(settings.controller.clockFontFamily))
                            onActivated: settings.controller.clockFontFamily = currentText
                        }
                        Button {
                            text: qsTr("N")
                            Accessible.name: qsTr("Negrito")
                            checkable: true
                            checked: settings.controller.clockFontBold
                            font.bold: true
                            onClicked: settings.controller.clockFontBold = checked
                        }
                        Button {
                            text: qsTr("I")
                            Accessible.name: qsTr("Itálico")
                            checkable: true
                            checked: settings.controller.clockFontItalic
                            font.italic: true
                            onClicked: settings.controller.clockFontItalic = checked
                        }
                    }
                    RowLayout {
                        Label { text: qsTr("Cor da fonte"); color: settings.textMain; Layout.preferredWidth: 160 }
                        TextField { Layout.fillWidth: true; text: settings.controller.clockColor; onEditingFinished: settings.controller.clockColor = text }
                    }
                    RowLayout {
                        Label { text: qsTr("Cor de fundo"); color: settings.textMain; Layout.preferredWidth: 160 }
                        TextField { Layout.fillWidth: true; text: settings.controller.clockBackgroundColor; onEditingFinished: settings.controller.clockBackgroundColor = text }
                    }
                    RowLayout {
                        Label { text: qsTr("Altura da linha"); color: settings.textMain; Layout.preferredWidth: 160 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [{"id":0.75,"name":"75%"},{"id":1.0,"name":"100%"},{"id":1.15,"name":"115%"},{"id":1.25,"name":"125%"},{"id":1.5,"name":"150%"},{"id":2.0,"name":"200%"}]
                            textRole: "name"; valueRole: "id"
                            currentIndex: settings.valueIndex(model, settings.controller.clockLineHeight)
                            onActivated: settings.controller.clockLineHeight = currentValue
                        }
                    }
                    RowLayout {
                        Label { text: qsTr("Efeito"); color: settings.textMain; Layout.preferredWidth: 160 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [{"id":"none","name":qsTr("Nenhum")},
                                    {"id":"outline","name":qsTr("Contorno")},
                                    {"id":"raised","name":qsTr("Elevado")},
                                    {"id":"sunken","name":qsTr("Baixo relevo")}]
                            textRole: "name"; valueRole: "id"
                            currentIndex: settings.valueIndex(model, settings.controller.clockEffect)
                            onActivated: settings.controller.clockEffect = currentValue
                        }
                        Label { text: qsTr("Canto"); color: settings.textMain }
                        SpinBox { from: 0; to: 200; value: settings.controller.clockCornerRadius; onValueModified: settings.controller.clockCornerRadius = value }
                    }
                    RowLayout {
                        Label { text: qsTr("Opacidade da fonte"); color: settings.textMain; Layout.preferredWidth: 160 }
                        SpinBox { from: 0; to: 100; value: Math.round(settings.controller.clockTextOpacity * 100); onValueModified: settings.controller.clockTextOpacity = value / 100 }
                        Label { text: "%"; color: settings.textMuted }
                        Label { text: qsTr("Opacidade do fundo"); color: settings.textMain }
                        SpinBox { from: 0; to: 100; value: Math.round(settings.controller.clockBackgroundOpacity * 100); onValueModified: settings.controller.clockBackgroundOpacity = value / 100 }
                        Label { text: "%"; color: settings.textMuted }
                    }
                    RowLayout {
                        Label { text: qsTr("Localização"); color: settings.textMain; Layout.preferredWidth: 160 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [{"id":"topLeft","name":qsTr("Superior esquerda")},
                                    {"id":"topCenter","name":qsTr("Superior centro")},
                                    {"id":"topRight","name":qsTr("Superior direita")},
                                    {"id":"centerLeft","name":qsTr("Centro esquerda")},
                                    {"id":"center","name":qsTr("Centro")},
                                    {"id":"centerRight","name":qsTr("Centro direita")},
                                    {"id":"bottomLeft","name":qsTr("Inferior esquerda")},
                                    {"id":"bottomCenter","name":qsTr("Inferior centro")},
                                    {"id":"bottomRight","name":qsTr("Inferior direita")}]
                            textRole: "name"; valueRole: "id"
                            currentIndex: settings.valueIndex(model, settings.controller.clockPosition)
                            onActivated: settings.controller.clockPosition = currentValue
                        }
                    }
                    RowLayout {
                        Label { text: qsTr("Margem horizontal (%)"); color: settings.textMain; Layout.preferredWidth: 160 }
                        SpinBox { from: -100; to: 100; value: settings.controller.clockMarginHorizontal; onValueModified: settings.controller.clockMarginHorizontal = value }
                        Label { text: qsTr("Margem vertical (%)"); color: settings.textMain }
                        SpinBox { from: -100; to: 100; value: settings.controller.clockMarginVertical; onValueModified: settings.controller.clockMarginVertical = value }
                    }
                }
            }

            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 12
                    Label { text: qsTr("Controle remoto local"); color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("O servidor fica desligado por padrão e deve ser usado somente na rede local confiável.")
                        color: settings.textMuted
                        wrapMode: Text.WordWrap
                    }
                    CheckBox {
                        text: qsTr("Habilitar controle remoto")
                        checked: settings.controller.remoteEnabled
                        enabled: settings.controller.remotePasswordConfigured
                        palette.windowText: settings.textMain
                        onClicked: settings.controller.remoteEnabled = checked
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Interface IPv4"); color: settings.textMain; Layout.preferredWidth: 170 }
                        ComboBox {
                            id: remoteInterfacePicker
                            Layout.fillWidth: true
                            editable: true
                            model: ["0.0.0.0", "127.0.0.1"]
                            editText: settings.controller.remoteInterface
                            onAccepted: settings.controller.remoteInterface = editText
                            onActivated: settings.controller.remoteInterface = currentText
                        }
                    }
                    RowLayout {
                        Label { text: qsTr("Porta"); color: settings.textMain; Layout.preferredWidth: 170 }
                        SpinBox {
                            from: 1024; to: 65535
                            value: settings.controller.remotePort
                            onValueModified: settings.controller.remotePort = value
                        }
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label { text: qsTr("Senha fixa"); color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            id: remotePasswordField
                            Layout.fillWidth: true
                            echoMode: TextInput.Password
                            placeholderText: settings.controller.remotePasswordConfigured
                                             ? qsTr("Nova senha (mínimo 8 caracteres)")
                                             : qsTr("Defina uma senha (mínimo 8 caracteres)")
                            onAccepted: saveRemotePasswordButton.clicked()
                        }
                        Button {
                            id: saveRemotePasswordButton
                            text: settings.controller.remotePasswordConfigured
                                  ? qsTr("Trocar senha") : qsTr("Definir senha")
                            enabled: remotePasswordField.text.length >= 8
                            onClicked: {
                                if (settings.controller.setRemotePassword(remotePasswordField.text))
                                    remotePasswordField.clear()
                            }
                        }
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label { text: qsTr("Acesso"); color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    Label {
                        Layout.fillWidth: true
                        text: settings.controller.remoteEnabled
                              ? settings.controller.remoteUrl : qsTr("Servidor remoto desabilitado")
                        color: settings.controller.remoteEnabled ? "#70e1a7" : settings.textMuted
                        wrapMode: Text.WrapAnywhere
                        font.bold: settings.controller.remoteEnabled
                    }
                    Image {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 196
                        Layout.preferredHeight: 196
                        visible: settings.controller.remoteEnabled
                                 && settings.controller.remoteQrCode.length > 0
                        source: settings.controller.remoteQrCode
                        fillMode: Image.PreserveAspectFit
                        cache: false
                    }
                    Label {
                        visible: settings.controller.remoteEnabled
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Leia o QR na mesma rede local para abrir o controle.")
                        color: settings.textMuted
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        text: qsTr("Clientes conectados: %1  •  Sessões ativas: %2")
                              .arg(settings.controller.remoteClients)
                              .arg(settings.controller.remoteSessions)
                        color: settings.textMuted
                    }
                    Button {
                        text: qsTr("Revogar todas as sessões")
                        enabled: settings.controller.remoteSessions > 0
                        onClicked: settings.controller.revokeRemoteSessions()
                    }
                    Label {
                        visible: settings.controller.remoteError.length > 0
                        Layout.fillWidth: true
                        text: settings.controller.remoteError
                        color: "#ff8f8f"
                        wrapMode: Text.WordWrap
                    }
                }
            }

            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 12
                    Label { text: qsTr("Opções de desenvolvimento"); color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    CheckBox {
                        text: qsTr("Ativar modo de debug")
                        checked: settings.controller.debugEnabled
                        palette.windowText: settings.textMain
                        onClicked: settings.controller.debugEnabled = checked
                    }
                    CheckBox {
                        text: qsTr("Múltiplas saídas simuladas")
                        enabled: settings.controller.debugEnabled
                        checked: settings.controller.debugSimulatedOutputs
                        palette.windowText: settings.textMain
                        onClicked: settings.controller.debugSimulatedOutputs = checked
                    }
                    CheckBox {
                        text: qsTr("Diagnósticos e benchmark")
                        enabled: settings.controller.debugEnabled
                        checked: settings.controller.debugDiagnostics
                        palette.windowText: settings.textMain
                        onClicked: settings.controller.debugDiagnostics = checked
                    }
                    CheckBox {
                        text: qsTr("Registrar mensagens DEBUG")
                        enabled: settings.controller.debugEnabled
                        checked: settings.controller.debugLogging
                        palette.windowText: settings.textMain
                        onClicked: settings.controller.debugLogging = checked
                    }
                    RowLayout {
                        enabled: settings.controller.debugEnabled
                        Label { text: qsTr("Saídas simuladas"); color: settings.textMain; Layout.preferredWidth: 180 }
                        SpinBox { from: 1; to: 5; value: settings.controller.simulatedOutputCount; onValueModified: settings.controller.simulatedOutputCount = value }
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Os registros continuam sendo gravados no diretório de dados do HolyScreen, mesmo sem a janela de terminal.")
                        color: settings.textMuted
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }

    FileDialog {
        id: importProfileDialog
        title: qsTr("Importar perfil do HolyScreen")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Perfil HolyScreen (*.json)")]
        onAccepted: {
            const result = settings.controller.importConfiguration(selectedFile)
            settings.profileStatus = result.accepted
                    ? qsTr("Perfil importado. Credenciais e servidor remoto foram preservados.")
                    : (result.errors || [qsTr("Não foi possível importar o perfil.")]).join("\n")
        }
    }

    FileDialog {
        id: exportProfileDialog
        title: qsTr("Exportar perfil do HolyScreen")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("Perfil HolyScreen (*.json)")]
        onAccepted: {
            const result = settings.controller.exportConfiguration(selectedFile)
            settings.profileStatus = result.accepted
                    ? qsTr("Perfil exportado sem segredos.")
                    : (result.errors || [qsTr("Não foi possível exportar o perfil.")]).join("\n")
        }
    }
}
