pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Dialog {
    id: editor

    required property var controller
    property string scope: "bible"
    property string selectedThemeId: ""

    title: scope === "bible" ? qsTr("Temas da Bíblia") : qsTr("Temas de letras")
    modal: true
    width: Math.min(920, parent ? parent.width - 80 : 920)
    height: Math.min(720, parent ? parent.height - 80 : 720)
    anchors.centerIn: parent
    standardButtons: Dialog.Close

    readonly property color panel: "#20252a"
    readonly property color panelHigh: "#2a3036"
    readonly property color line: "#46505a"
    readonly property color textMain: "#f2f4f5"
    readonly property color textMuted: "#aab2b8"

    function themeIndex(id) {
        for (let index = 0; index < controller.themes.length; ++index) {
            if (controller.themes[index].id === id)
                return index
        }
        return controller.themes.length > 0 ? 0 : -1
    }

    function valueIndex(model, value) {
        for (let index = 0; index < model.length; ++index) {
            if (model[index].value === value)
                return index
        }
        return 0
    }

    function loadTheme(theme) {
        if (!theme || !theme.id)
            return
        selectedThemeId = theme.id
        themeName.text = theme.name || ""
        backgroundType.currentIndex = theme.backgroundType === 2 ? 1 : 0
        backgroundColor.text = theme.backgroundColor || "#000000"
        backgroundImage.text = theme.backgroundImage || ""
        fontPicker.currentIndex = Math.max(0, fontPicker.model.indexOf(theme.fontFamily || ""))
        fontSize.value = theme.fontSize || 72
        minimumFontSize.value = theme.minimumFontSize || 28
        fontWeight.currentIndex = valueIndex(fontWeight.model, theme.fontWeight || 700)
        textColor.text = theme.textColor || "#ffffff"
        horizontalAlignment.currentIndex = valueIndex(
                    horizontalAlignment.model, theme.horizontalAlignment || "center")
        verticalAlignment.currentIndex = valueIndex(
                    verticalAlignment.model, theme.verticalAlignment || "center")
        lineSpacing.value = theme.lineSpacing || 0
        textMargin.value = theme.margin || 0
        outline.checked = theme.outline === true
        outlineColor.text = theme.outlineColor || "#a0000000"
        shadow.checked = theme.shadow === true
        shadowColor.text = theme.shadowColor || "#80000000"
    }

    function syncSelection() {
        const preferredId = scope === "bible"
                ? controller.bibleThemeId : controller.lyricsThemeId
        const index = themeIndex(preferredId)
        themePicker.currentIndex = index
        if (index >= 0)
            loadTheme(controller.themes[index])
    }

    function openFor(requestedScope) {
        scope = requestedScope === "lyrics" ? "lyrics" : "bible"
        syncSelection()
        open()
    }

    function saveTheme() {
        if (!selectedThemeId.length)
            return
        controller.updateThemeById(selectedThemeId, {
            "name": themeName.text.trim(),
            "backgroundType": backgroundType.currentValue,
            "backgroundColor": backgroundColor.text.trim(),
            "backgroundImage": backgroundImage.text.trim(),
            "fontFamily": fontPicker.currentText,
            "fontSize": fontSize.value,
            "minimumFontSize": minimumFontSize.value,
            "fontWeight": fontWeight.currentValue,
            "textColor": textColor.text.trim(),
            "horizontalAlignment": horizontalAlignment.currentValue,
            "verticalAlignment": verticalAlignment.currentValue,
            "lineSpacing": lineSpacing.value,
            "margin": textMargin.value,
            "outline": outline.checked,
            "outlineColor": outlineColor.text.trim(),
            "shadow": shadow.checked,
            "shadowColor": shadowColor.text.trim(),
            "transition": "fade"
        })
        controller.applyThemeForContent(scope, selectedThemeId)
        Qt.callLater(syncSelection)
    }

    background: Rectangle {
        color: editor.panel
        border.color: editor.line
        radius: 8
    }

    contentItem: ColumnLayout {
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: editor.scope === "bible" ? qsTr("Tema padrão da Bíblia")
                                               : qsTr("Tema padrão das letras")
                color: editor.textMain
                font.bold: true
            }
            ComboBox {
                id: themePicker
                Layout.fillWidth: true
                model: editor.controller.themes
                textRole: "name"
                valueRole: "id"
                onActivated: editor.loadTheme(editor.controller.themes[currentIndex])
            }
            Button {
                text: qsTr("Novo")
                onClicked: {
                    const id = editor.controller.createTheme(qsTr("Novo tema"))
                    if (id.length) {
                        const index = editor.themeIndex(id)
                        themePicker.currentIndex = index
                        if (index >= 0)
                            editor.loadTheme(editor.controller.themes[index])
                    }
                }
            }
            Button {
                text: qsTr("Excluir")
                enabled: editor.controller.themes.length > 1
                         && editor.selectedThemeId.length > 0
                onClicked: {
                    editor.controller.deleteTheme(editor.selectedThemeId)
                    Qt.callLater(editor.syncSelection)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ColumnLayout {
                    width: parent.width
                    spacing: 9
                    Label { text: qsTr("Nome"); color: editor.textMain; font.bold: true }
                    TextField { id: themeName; Layout.fillWidth: true }
                    Label { text: qsTr("Fundo"); color: editor.textMain; font.bold: true }
                    ComboBox {
                        id: backgroundType
                        Layout.fillWidth: true
                        model: [{"text":qsTr("Cor sólida"), "value":0},
                                {"text":qsTr("Imagem"), "value":2}]
                        textRole: "text"; valueRole: "value"
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Cor"); color: editor.textMuted; Layout.preferredWidth: 90 }
                        TextField { id: backgroundColor; Layout.fillWidth: true }
                    }
                    RowLayout {
                        visible: backgroundType.currentValue === 2
                        Layout.fillWidth: true
                        Label { text: qsTr("Imagem"); color: editor.textMuted; Layout.preferredWidth: 90 }
                        TextField { id: backgroundImage; Layout.fillWidth: true }
                        Button { text: qsTr("Escolher"); onClicked: imageDialog.open() }
                    }
                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: editor.line }
                    Label { text: qsTr("Texto"); color: editor.textMain; font.bold: true }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Fonte"); color: editor.textMuted; Layout.preferredWidth: 90 }
                        ComboBox { id: fontPicker; Layout.fillWidth: true; model: Qt.fontFamilies() }
                    }
                    RowLayout {
                        Label { text: qsTr("Tamanho"); color: editor.textMuted; Layout.preferredWidth: 90 }
                        SpinBox { id: fontSize; from: 28; to: 240 }
                        Label { text: qsTr("Mínimo"); color: editor.textMuted }
                        SpinBox { id: minimumFontSize; from: 12; to: fontSize.value }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Peso"); color: editor.textMuted; Layout.preferredWidth: 90 }
                        ComboBox {
                            id: fontWeight
                            Layout.fillWidth: true
                            model: [{"text":qsTr("Normal"), "value":400},
                                    {"text":qsTr("Seminegrito"), "value":600},
                                    {"text":qsTr("Negrito"), "value":700}]
                            textRole: "text"; valueRole: "value"
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Cor do texto"); color: editor.textMuted; Layout.preferredWidth: 90 }
                        TextField { id: textColor; Layout.fillWidth: true }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Horizontal"); color: editor.textMuted; Layout.preferredWidth: 90 }
                        ComboBox {
                            id: horizontalAlignment
                            Layout.fillWidth: true
                            model: [{"text":qsTr("Esquerda"), "value":"left"},
                                    {"text":qsTr("Centro"), "value":"center"},
                                    {"text":qsTr("Direita"), "value":"right"}]
                            textRole: "text"; valueRole: "value"
                        }
                        Label { text: qsTr("Vertical"); color: editor.textMuted }
                        ComboBox {
                            id: verticalAlignment
                            Layout.fillWidth: true
                            model: [{"text":qsTr("Superior"), "value":"top"},
                                    {"text":qsTr("Centro"), "value":"center"},
                                    {"text":qsTr("Inferior"), "value":"bottom"}]
                            textRole: "text"; valueRole: "value"
                        }
                    }
                    RowLayout {
                        Label { text: qsTr("Espaçamento"); color: editor.textMuted; Layout.preferredWidth: 90 }
                        SpinBox { id: lineSpacing; from: -50; to: 200 }
                        Label { text: qsTr("Margem"); color: editor.textMuted }
                        SpinBox { id: textMargin; from: 0; to: 400 }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        CheckBox { id: outline; text: qsTr("Contorno"); palette.windowText: editor.textMain }
                        TextField { id: outlineColor; Layout.fillWidth: true; enabled: outline.checked }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        CheckBox { id: shadow; text: qsTr("Sombra"); palette.windowText: editor.textMain }
                        TextField { id: shadowColor; Layout.fillWidth: true; enabled: shadow.checked }
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 340
                Layout.fillHeight: true
                color: backgroundColor.text || "#000000"
                border.color: editor.line
                radius: 6
                clip: true
                Image {
                    anchors.fill: parent
                    visible: backgroundType.currentValue === 2
                    source: backgroundImage.text
                    fillMode: Image.PreserveAspectCrop
                }
                Label {
                    anchors.fill: parent
                    anchors.margins: Math.min(60, textMargin.value / 2)
                    text: editor.scope === "bible"
                          ? qsTr("Porque Deus amou o mundo de tal maneira…")
                          : qsTr("Esta é uma prévia da letra\nna tela de apresentação")
                    color: textColor.text || "#ffffff"
                    font.family: fontPicker.currentText
                    font.pixelSize: Math.min(42, fontSize.value / 2)
                    font.weight: fontWeight.currentValue
                    wrapMode: Text.WordWrap
                    horizontalAlignment: horizontalAlignment.currentValue === "left"
                                         ? Text.AlignLeft
                                         : horizontalAlignment.currentValue === "right"
                                           ? Text.AlignRight : Text.AlignHCenter
                    verticalAlignment: verticalAlignment.currentValue === "top"
                                       ? Text.AlignTop
                                       : verticalAlignment.currentValue === "bottom"
                                         ? Text.AlignBottom : Text.AlignVCenter
                    style: outline.checked ? Text.Outline
                           : shadow.checked ? Text.Raised : Text.Normal
                    styleColor: outline.checked ? outlineColor.text : shadowColor.text
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("Salvar e usar neste conteúdo")
                enabled: editor.selectedThemeId.length > 0
                highlighted: true
                onClicked: editor.saveTheme()
            }
        }
    }

    FileDialog {
        id: imageDialog
        title: qsTr("Selecionar imagem de fundo")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Imagens (*.jpg *.jpeg *.png *.webp)")]
        onAccepted: backgroundImage.text = selectedFile.toString()
    }
}
