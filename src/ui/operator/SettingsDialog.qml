pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: settings
    required property var controller
    signal openLibrary()
    signal chooseBackground()

    title: "Configurações"
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
            TabButton { text: "Geral"; palette.buttonText: settings.textMain }
            TabButton { text: "Telas"; palette.buttonText: settings.textMain }
            TabButton { text: "Mídia"; palette.buttonText: settings.textMain }
            TabButton { text: "Aparência"; palette.buttonText: settings.textMain }
            TabButton { text: "Remoto"; palette.buttonText: settings.textMain }
            TabButton { text: "Avançado"; palette.buttonText: settings.textMain }
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
                    Label { text: "Biblioteca"; color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    Label {
                        Layout.fillWidth: true
                        text: "Gerencie as pastas usadas pelo catálogo de áudio, vídeo e imagens."
                        color: settings.textMuted
                        wrapMode: Text.WordWrap
                    }
                    RowLayout {
                        Button { text: "Abrir biblioteca"; onClicked: settings.openLibrary() }
                        Button { text: "Reexaminar pastas"; onClicked: settings.controller.rescanMediaFolders() }
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label { text: "Comunicação com o palco"; color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    TextArea {
                        id: stageMessageSetting
                        Layout.fillWidth: true
                        Layout.preferredHeight: 90
                        text: settings.controller.stageMessage
                        placeholderText: "Mensagem para as telas configuradas como palco"
                        wrapMode: TextEdit.Wrap
                    }
                    Button {
                        text: "Salvar mensagem"
                        onClicked: settings.controller.stageMessage = stageMessageSetting.text
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label { text: "Atualizações e manutenção"; color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    TextField {
                        Layout.fillWidth: true
                        text: settings.controller.updateEndpoint
                        placeholderText: "Endereço de atualização"
                        onEditingFinished: settings.controller.updateEndpoint = text
                    }
                    RowLayout {
                        Button { text: "Verificar atualizações"; onClicked: settings.controller.checkForUpdates() }
                        Button { text: "Criar backup"; onClicked: settings.controller.createBackup() }
                    }
                    Label {
                        Layout.fillWidth: true
                        text: settings.controller.updateStatus.length > 0
                              ? settings.controller.updateStatus : settings.controller.autosaveStatus
                        color: settings.textMuted
                        wrapMode: Text.WordWrap
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
                        Label { text: "Monitores detectados"; color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                        Item { Layout.fillWidth: true }
                        Button { text: "Identificar"; onClicked: settings.controller.identifyScreens() }
                        Button { text: "Ativar todas"; onClicked: settings.controller.enableAllScreens() }
                    }
                    Repeater {
                        model: settings.controller.screens
                        delegate: Rectangle {
                            required property var modelData
                            Layout.fillWidth: true
                            implicitHeight: modelData.primary ? 64 : 116
                            radius: 6
                            color: settings.panelHigh
                            border.color: modelData.selected ? "#7294ff" : settings.line
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                RowLayout {
                                    Layout.fillWidth: true
                                    CheckBox {
                                        checked: modelData.primary || modelData.selected
                                        enabled: !modelData.primary
                                        onClicked: settings.controller.toggleScreen(modelData.id, checked)
                                    }
                                    TextField {
                                        Layout.fillWidth: true
                                        text: modelData.name
                                        readOnly: modelData.primary || !modelData.selected
                                        onEditingFinished: settings.controller.setOutputDisplayName(modelData.id, text)
                                    }
                                    Label {
                                        text: modelData.primary ? "OPERADOR" : modelData.selected ? "ATIVA" : "INATIVA"
                                        color: modelData.selected ? "#9eb5ff" : settings.textMuted
                                        font.bold: true
                                        font.pixelSize: 10
                                    }
                                }
                                RowLayout {
                                    visible: !modelData.primary && modelData.selected
                                    Layout.fillWidth: true
                                    CheckBox {
                                        text: "Exibir vídeo"
                                        checked: modelData.mediaEnabled
                                        palette.windowText: settings.textMain
                                        onClicked: settings.controller.setOutputMediaEnabled(modelData.id, checked)
                                    }
                                    ComboBox {
                                        Layout.preferredWidth: 210
                                        model: [{"id":"audience", "name":"Saída para o público"},
                                                {"id":"stage", "name":"Saída de palco"}]
                                        textRole: "name"
                                        valueRole: "id"
                                        currentIndex: settings.valueIndex(model, modelData.role)
                                        onActivated: settings.controller.setOutputRole(modelData.id, currentValue)
                                    }
                                    Item { Layout.fillWidth: true }
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
                    Label { text: "Reprodução"; color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    RowLayout {
                        Label { text: "Volume"; color: settings.textMain; Layout.preferredWidth: 160 }
                        Slider { Layout.fillWidth: true; from: 0; to: 1; value: settings.controller.mediaVolume; onMoved: settings.controller.mediaVolume = value }
                    }
                    RowLayout {
                        Label { text: "Repetição"; color: settings.textMain; Layout.preferredWidth: 160 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [{"id":"off", "name":"Sem repetição"}, {"id":"one", "name":"Repetir item"}, {"id":"all", "name":"Repetir playlist"}]
                            textRole: "name"; valueRole: "id"
                            currentIndex: settings.valueIndex(model, settings.controller.mediaRepeatMode)
                            onActivated: settings.controller.mediaRepeatMode = currentValue
                        }
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label { text: "Imagens"; color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    RowLayout {
                        Label { text: "Ajuste"; color: settings.textMain; Layout.preferredWidth: 160 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [{"id":"contain", "name":"Conter"}, {"id":"cover", "name":"Preencher"}, {"id":"stretch", "name":"Esticar"}, {"id":"center", "name":"Centralizar"}]
                            textRole: "name"; valueRole: "id"
                            currentIndex: settings.valueIndex(model, settings.controller.imageFit)
                            onActivated: settings.controller.imageFit = currentValue
                        }
                    }
                    CheckBox {
                        text: "Avançar imagens automaticamente"
                        checked: settings.controller.imageAutoplay
                        palette.windowText: settings.textMain
                        onClicked: settings.controller.imageAutoplay = checked
                    }
                    RowLayout {
                        enabled: settings.controller.imageAutoplay
                        Label { text: "Intervalo (segundos)"; color: settings.textMain; Layout.preferredWidth: 160 }
                        SpinBox { from: 1; to: 3600; value: Math.round(settings.controller.imageIntervalMs / 1000); onValueModified: settings.controller.imageIntervalMs = value * 1000 }
                    }
                }
            }

            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 14
                    Label { text: "Fundo da apresentação"; color: settings.textMain; font.bold: true; font.pixelSize: 16 }
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
                            text: "Fundo de cor sólida"
                            color: settings.textMain
                            style: Text.Outline
                            styleColor: "#80000000"
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: "Escolher imagem…"; onClicked: settings.chooseBackground() }
                        Button {
                            text: "Remover imagem"
                            enabled: settings.controller.wallpaperSource.toString().length > 0
                            onClicked: settings.controller.wallpaperSource = ""
                        }
                        Label {
                            Layout.fillWidth: true
                            text: settings.controller.wallpaperSource.toString().length > 0
                                  ? settings.controller.wallpaperSource.toString() : "Nenhuma imagem selecionada"
                            color: settings.textMuted
                            elide: Text.ElideMiddle
                        }
                    }
                    RowLayout {
                        Label { text: "Cor"; color: settings.textMain; Layout.preferredWidth: 160 }
                        TextField { Layout.fillWidth: true; text: settings.controller.wallpaperColor; placeholderText: "#000000"; onEditingFinished: settings.controller.wallpaperColor = text }
                    }
                    RowLayout {
                        Label { text: "Ajuste do fundo"; color: settings.textMain; Layout.preferredWidth: 160 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [{"id":"cover", "name":"Preencher"}, {"id":"contain", "name":"Conter"}, {"id":"stretch", "name":"Esticar"}, {"id":"center", "name":"Centralizar"}]
                            textRole: "name"; valueRole: "id"
                            currentIndex: settings.valueIndex(model, settings.controller.wallpaperFit)
                            onActivated: settings.controller.wallpaperFit = currentValue
                        }
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label { text: "Relógio"; color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    CheckBox {
                        text: "Exibir relógio nas saídas"
                        checked: settings.controller.clockVisible
                        palette.windowText: settings.textMain
                        onClicked: settings.controller.clockVisible = checked
                    }
                    RowLayout {
                        Label { text: "Formato"; color: settings.textMain; Layout.preferredWidth: 160 }
                        TextField { Layout.fillWidth: true; text: settings.controller.clockFormat; onEditingFinished: settings.controller.clockFormat = text }
                    }
                    RowLayout {
                        Label { text: "Tamanho"; color: settings.textMain; Layout.preferredWidth: 160 }
                        SpinBox { from: 12; to: 300; value: settings.controller.clockFontSize; onValueModified: settings.controller.clockFontSize = value }
                    }
                    RowLayout {
                        Label { text: "Nome da fonte"; color: settings.textMain; Layout.preferredWidth: 160 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: Qt.fontFamilies()
                            currentIndex: Math.max(0, model.indexOf(settings.controller.clockFontFamily))
                            onActivated: settings.controller.clockFontFamily = currentText
                        }
                        Button {
                            text: "N"
                            checkable: true
                            checked: settings.controller.clockFontBold
                            font.bold: true
                            onClicked: settings.controller.clockFontBold = checked
                        }
                        Button {
                            text: "I"
                            checkable: true
                            checked: settings.controller.clockFontItalic
                            font.italic: true
                            onClicked: settings.controller.clockFontItalic = checked
                        }
                    }
                    RowLayout {
                        Label { text: "Cor da fonte"; color: settings.textMain; Layout.preferredWidth: 160 }
                        TextField { Layout.fillWidth: true; text: settings.controller.clockColor; onEditingFinished: settings.controller.clockColor = text }
                    }
                    RowLayout {
                        Label { text: "Cor de fundo"; color: settings.textMain; Layout.preferredWidth: 160 }
                        TextField { Layout.fillWidth: true; text: settings.controller.clockBackgroundColor; onEditingFinished: settings.controller.clockBackgroundColor = text }
                    }
                    RowLayout {
                        Label { text: "Altura da linha"; color: settings.textMain; Layout.preferredWidth: 160 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [{"id":0.75,"name":"75%"},{"id":1.0,"name":"100%"},{"id":1.15,"name":"115%"},{"id":1.25,"name":"125%"},{"id":1.5,"name":"150%"},{"id":2.0,"name":"200%"}]
                            textRole: "name"; valueRole: "id"
                            currentIndex: settings.valueIndex(model, settings.controller.clockLineHeight)
                            onActivated: settings.controller.clockLineHeight = currentValue
                        }
                    }
                    RowLayout {
                        Label { text: "Efeito"; color: settings.textMain; Layout.preferredWidth: 160 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [{"id":"none","name":"Nenhum"},{"id":"outline","name":"Contorno"},{"id":"raised","name":"Elevado"},{"id":"sunken","name":"Baixo relevo"}]
                            textRole: "name"; valueRole: "id"
                            currentIndex: settings.valueIndex(model, settings.controller.clockEffect)
                            onActivated: settings.controller.clockEffect = currentValue
                        }
                        Label { text: "Canto"; color: settings.textMain }
                        SpinBox { from: 0; to: 200; value: settings.controller.clockCornerRadius; onValueModified: settings.controller.clockCornerRadius = value }
                    }
                    RowLayout {
                        Label { text: "Opacidade da fonte"; color: settings.textMain; Layout.preferredWidth: 160 }
                        SpinBox { from: 0; to: 100; value: Math.round(settings.controller.clockTextOpacity * 100); onValueModified: settings.controller.clockTextOpacity = value / 100 }
                        Label { text: "%"; color: settings.textMuted }
                        Label { text: "Opacidade do fundo"; color: settings.textMain }
                        SpinBox { from: 0; to: 100; value: Math.round(settings.controller.clockBackgroundOpacity * 100); onValueModified: settings.controller.clockBackgroundOpacity = value / 100 }
                        Label { text: "%"; color: settings.textMuted }
                    }
                    RowLayout {
                        Label { text: "Localização"; color: settings.textMain; Layout.preferredWidth: 160 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: [{"id":"topLeft","name":"Superior esquerda"},{"id":"topCenter","name":"Superior centro"},{"id":"topRight","name":"Superior direita"},{"id":"centerLeft","name":"Centro esquerda"},{"id":"center","name":"Centro"},{"id":"centerRight","name":"Centro direita"},{"id":"bottomLeft","name":"Inferior esquerda"},{"id":"bottomCenter","name":"Inferior centro"},{"id":"bottomRight","name":"Inferior direita"}]
                            textRole: "name"; valueRole: "id"
                            currentIndex: settings.valueIndex(model, settings.controller.clockPosition)
                            onActivated: settings.controller.clockPosition = currentValue
                        }
                    }
                    RowLayout {
                        Label { text: "Margem horizontal (%)"; color: settings.textMain; Layout.preferredWidth: 160 }
                        SpinBox { from: -100; to: 100; value: settings.controller.clockMarginHorizontal; onValueModified: settings.controller.clockMarginHorizontal = value }
                        Label { text: "Margem vertical (%)"; color: settings.textMain }
                        SpinBox { from: -100; to: 100; value: settings.controller.clockMarginVertical; onValueModified: settings.controller.clockMarginVertical = value }
                    }
                }
            }

            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 12
                    Label { text: "Controle remoto local"; color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    Label {
                        Layout.fillWidth: true
                        text: "O servidor fica desligado por padrão e deve ser usado somente na rede local confiável."
                        color: settings.textMuted
                        wrapMode: Text.WordWrap
                    }
                    CheckBox {
                        text: "Habilitar controle remoto"
                        checked: settings.controller.remoteEnabled
                        enabled: settings.controller.remotePasswordConfigured
                        palette.windowText: settings.textMain
                        onClicked: settings.controller.remoteEnabled = checked
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "Interface IPv4"; color: settings.textMain; Layout.preferredWidth: 170 }
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
                        Label { text: "Porta"; color: settings.textMain; Layout.preferredWidth: 170 }
                        SpinBox {
                            from: 1024; to: 65535
                            value: settings.controller.remotePort
                            onValueModified: settings.controller.remotePort = value
                        }
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label { text: "Senha fixa"; color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            id: remotePasswordField
                            Layout.fillWidth: true
                            echoMode: TextInput.Password
                            placeholderText: settings.controller.remotePasswordConfigured
                                             ? "Nova senha (mínimo 8 caracteres)"
                                             : "Defina uma senha (mínimo 8 caracteres)"
                            onAccepted: saveRemotePasswordButton.clicked()
                        }
                        Button {
                            id: saveRemotePasswordButton
                            text: settings.controller.remotePasswordConfigured ? "Trocar senha" : "Definir senha"
                            enabled: remotePasswordField.text.length >= 8
                            onClicked: {
                                if (settings.controller.setRemotePassword(remotePasswordField.text))
                                    remotePasswordField.clear()
                            }
                        }
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label { text: "Acesso"; color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    Label {
                        Layout.fillWidth: true
                        text: settings.controller.remoteEnabled
                              ? settings.controller.remoteUrl : "Servidor remoto desabilitado"
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
                        text: "Leia o QR na mesma rede local para abrir o controle."
                        color: settings.textMuted
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        text: "Clientes conectados: " + settings.controller.remoteClients
                              + "  •  Sessões ativas: " + settings.controller.remoteSessions
                        color: settings.textMuted
                    }
                    Button {
                        text: "Revogar todas as sessões"
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
                    Label { text: "Opções de desenvolvimento"; color: settings.textMain; font.bold: true; font.pixelSize: 16 }
                    CheckBox {
                        text: "Ativar modo de debug"
                        checked: settings.controller.debugEnabled
                        palette.windowText: settings.textMain
                        onClicked: settings.controller.debugEnabled = checked
                    }
                    CheckBox {
                        text: "Múltiplas saídas simuladas"
                        enabled: settings.controller.debugEnabled
                        checked: settings.controller.debugSimulatedOutputs
                        palette.windowText: settings.textMain
                        onClicked: settings.controller.debugSimulatedOutputs = checked
                    }
                    CheckBox {
                        text: "Diagnósticos e benchmark"
                        enabled: settings.controller.debugEnabled
                        checked: settings.controller.debugDiagnostics
                        palette.windowText: settings.textMain
                        onClicked: settings.controller.debugDiagnostics = checked
                    }
                    CheckBox {
                        text: "Registrar mensagens DEBUG"
                        enabled: settings.controller.debugEnabled
                        checked: settings.controller.debugLogging
                        palette.windowText: settings.textMain
                        onClicked: settings.controller.debugLogging = checked
                    }
                    RowLayout {
                        enabled: settings.controller.debugEnabled
                        Label { text: "Saídas simuladas"; color: settings.textMain; Layout.preferredWidth: 180 }
                        SpinBox { from: 1; to: 5; value: settings.controller.simulatedOutputCount; onValueModified: settings.controller.simulatedOutputCount = value }
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: settings.line }
                    Label {
                        Layout.fillWidth: true
                        text: "Os registros continuam sendo gravados no diretório de dados do HolyScreen, mesmo sem a janela de terminal."
                        color: settings.textMuted
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }
}
