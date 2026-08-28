pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog

    required property var controller
    required property real hostWidth
    required property real hostHeight

    property string selectedKey: ""
    property bool draftDirty: false
    readonly property var selectedResult: {
        dialog.controller.onlineLyricsResults
        return dialog.controller.onlineLyricsResult(dialog.selectedKey)
    }

    title: qsTr("Buscar e editar letra online")
    modal: true
    width: Math.min(dialog.hostWidth - 60, 1120)
    height: Math.min(dialog.hostHeight - 60, 720)
    x: (dialog.hostWidth - width) / 2
    y: (dialog.hostHeight - height) / 2
    standardButtons: Dialog.Close

    function populateDraft(item) {
        if (!item || !item.key) return
        titleField.text = item.title || ""
        artistField.text = item.artist || ""
        lyricsEditor.text = item.lyrics || ""
        dialog.draftDirty = false
    }

    function selectResult(item) {
        if (!item || !item.key) return
        dialog.selectedKey = item.key
        dialog.populateDraft(item)
        if (!item.hasLyrics)
            dialog.controller.loadOnlineLyrics(item.key)
    }

    function openFor(key, query) {
        dialog.selectedKey = key || ""
        searchField.text = query || ""
        if (dialog.selectedKey.length > 0) {
            const item = dialog.controller.onlineLyricsResult(dialog.selectedKey)
            dialog.selectResult(item)
        } else if (searchField.text.trim().length >= 3) {
            dialog.controller.searchOnlineLyrics(searchField.text)
        }
        dialog.open()
        searchField.forceActiveFocus()
    }

    Connections {
        target: dialog.controller
        function onOnlineLyricsLoaded(key) {
            if (key === dialog.selectedKey && !dialog.draftDirty)
                dialog.populateDraft(dialog.controller.onlineLyricsResult(key))
        }
    }

    Timer {
        id: dialogSearchDebounce
        interval: 500
        repeat: false
        onTriggered: dialog.controller.searchOnlineLyrics(searchField.text)
    }

    contentItem: ColumnLayout {
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: searchField
                objectName: "onlineLyricsSearchField"
                Layout.fillWidth: true
                placeholderText: qsTr("Nome da música, artista ou trecho")
                Accessible.name: qsTr("Pesquisar letras online")
                onTextEdited: {
                    if (text.trim().length >= 3) dialogSearchDebounce.restart()
                    else {
                        dialogSearchDebounce.stop()
                        dialog.controller.cancelOnlineLyricsSearch()
                    }
                }
                onAccepted: {
                    if (text.trim().length >= 3)
                        dialog.controller.searchOnlineLyrics(text)
                }
            }
            Button {
                text: qsTr("BUSCAR")
                enabled: searchField.text.trim().length >= 3
                         && !dialog.controller.onlineLyricsBusy
                onClicked: dialog.controller.searchOnlineLyrics(searchField.text)
            }
            BusyIndicator {
                running: dialog.controller.onlineLyricsBusy
                visible: running
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            ColumnLayout {
                SplitView.preferredWidth: 360
                SplitView.minimumWidth: 280
                Label {
                    text: qsTr("RESULTADOS ONLINE")
                    font.bold: true
                    color: "#b9c7ff"
                }
                ListView {
                    id: onlineResults
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 5
                    model: dialog.controller.onlineLyricsResults
                    delegate: Rectangle {
                        id: resultDelegate
                        required property var modelData
                        width: ListView.view.width
                        height: 68
                        radius: 5
                        color: dialog.selectedKey === resultDelegate.modelData.key
                               ? "#344d78" : "#202a38"
                        border.color: dialog.selectedKey === resultDelegate.modelData.key
                                      ? "#b9c7ff" : "#41484e"
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 9
                            spacing: 2
                            Label {
                                Layout.fillWidth: true
                                text: resultDelegate.modelData.title
                                color: "#f2f4f5"
                                font.bold: true
                                elide: Text.ElideRight
                            }
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("%1 · %2")
                                          .arg(resultDelegate.modelData.artist)
                                          .arg(resultDelegate.modelData.provider)
                                color: "#aab2b8"
                                elide: Text.ElideRight
                            }
                        }
                        TapHandler {
                            onTapped: dialog.selectResult(resultDelegate.modelData)
                        }
                    }
                    Label {
                        anchors.centerIn: parent
                        visible: onlineResults.count === 0
                                 && !dialog.controller.onlineLyricsBusy
                        text: qsTr("Pesquise para encontrar letras na internet")
                        color: "#8d979f"
                    }
                }

                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Fallback Vagalume")
                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        Label {
                            Layout.fillWidth: true
                            text: dialog.controller.vagalumeApiKeyConfigured
                                  ? qsTr("Chave configurada no %1").arg(
                                        dialog.controller.lyricsSecretStorageName)
                                  : qsTr("Usado somente quando a LRCLIB não encontrar resultados")
                            color: "#aab2b8"
                            wrapMode: Text.Wrap
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            TextField {
                                id: vagalumeKey
                                Layout.fillWidth: true
                                echoMode: TextInput.Password
                                placeholderText: qsTr("Chave de API do Vagalume")
                                Accessible.name: qsTr("Chave de API do Vagalume")
                            }
                            Button {
                                text: qsTr("GUARDAR")
                                enabled: vagalumeKey.text.trim().length > 0
                                onClicked: {
                                    if (dialog.controller.setVagalumeApiKey(vagalumeKey.text))
                                        vagalumeKey.text = ""
                                }
                            }
                            Button {
                                text: qsTr("REMOVER")
                                enabled: dialog.controller.vagalumeApiKeyConfigured
                                onClicked: dialog.controller.clearVagalumeApiKey()
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                SplitView.fillWidth: true
                SplitView.minimumWidth: 420
                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: titleField
                        objectName: "onlineLyricsTitleField"
                        Layout.fillWidth: true
                        placeholderText: qsTr("Título")
                        Accessible.name: qsTr("Título da música")
                        onTextEdited: dialog.draftDirty = true
                    }
                    TextField {
                        id: artistField
                        objectName: "onlineLyricsArtistField"
                        Layout.fillWidth: true
                        placeholderText: qsTr("Artista ou autor")
                        Accessible.name: qsTr("Artista ou autor")
                        onTextEdited: dialog.draftDirty = true
                    }
                }
                TextArea {
                    id: lyricsEditor
                    objectName: "onlineLyricsEditor"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    placeholderText: qsTr("Selecione um resultado para revisar e editar a letra")
                    Accessible.name: qsTr("Editor da letra")
                    wrapMode: TextEdit.Wrap
                    selectByMouse: true
                    onTextChanged: if (activeFocus) dialog.draftDirty = true
                    background: Rectangle {
                        color: "#151b22"
                        border.color: lyricsEditor.activeFocus ? "#b9c7ff" : "#41484e"
                        radius: 5
                    }
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Use [Verso], [Refrão] e [Ponte] para controlar as seções. Sem marcações, os parágrafos serão separados automaticamente.")
                    color: "#aab2b8"
                    wrapMode: Text.Wrap
                }
                Label {
                    Layout.fillWidth: true
                    visible: dialog.controller.onlineLyricsError.length > 0
                    text: dialog.controller.onlineLyricsError
                    color: "#ffba70"
                    wrapMode: Text.Wrap
                }
                Label {
                    Layout.fillWidth: true
                    visible: dialog.controller.onlineLyricsStatus.length > 0
                    text: dialog.controller.onlineLyricsStatus
                    color: "#70e1a7"
                    wrapMode: Text.Wrap
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        text: qsTr("ABRIR FONTE")
                        enabled: dialog.selectedKey.length > 0
                        onClicked: dialog.controller.openOnlineLyricsSource(dialog.selectedKey)
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        objectName: "saveOnlineLyricsButton"
                        text: qsTr("SALVAR NA BIBLIOTECA")
                        highlighted: true
                        enabled: dialog.selectedKey.length > 0
                                 && titleField.text.trim().length > 0
                                 && lyricsEditor.text.trim().length > 0
                                 && !dialog.controller.onlineLyricsBusy
                        onClicked: {
                            const id = dialog.controller.saveEditedOnlineLyrics(
                                           dialog.selectedKey, titleField.text,
                                           artistField.text, lyricsEditor.text)
                            if (id.length > 0) dialog.draftDirty = false
                        }
                    }
                }
            }
        }
    }
}
