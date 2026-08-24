pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Dialog {
    id: dialog

    required property var controller
    required property real hostWidth
    required property real hostHeight

    title: qsTr("Biblioteca de pastas")
    modal: true
    width: Math.min(dialog.hostWidth - 80, 1000)
    height: Math.min(dialog.hostHeight - 80, 680)
    x: (dialog.hostWidth - width) / 2
    y: (dialog.hostHeight - height) / 2
    standardButtons: Dialog.Close

    FolderDialog {
        id: mediaFolderDialog
        title: qsTr("Adicionar pasta de mídia")
        onAccepted: dialog.controller.addMediaFolder(selectedFolder)
    }

    contentItem: ColumnLayout {
        spacing: 10
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("PASTAS SELECIONADAS")
                color: "#8da0bc"
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("+ PASTA")
                onClicked: mediaFolderDialog.open()
            }
            Button {
                text: qsTr("ATUALIZAR")
                onClicked: dialog.controller.rescanMediaFolders()
            }
        }
        ListView {
            id: mediaFoldersList
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(120, Math.max(42, contentHeight))
            clip: true
            spacing: 4
            model: dialog.controller.mediaFolders
            delegate: Rectangle {
                id: mediaFolderDelegate
                required property var modelData
                width: ListView.view.width
                height: 38
                radius: 5
                color: "#142137"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 6
                    Label {
                        text: mediaFolderDelegate.modelData.exists ? "●" : "!"
                        color: mediaFolderDelegate.modelData.exists ? "#70e1a7" : "#ffba70"
                    }
                    Label {
                        Layout.fillWidth: true
                        text: mediaFolderDelegate.modelData.path
                        color: "#d9e5f5"
                        elide: Text.ElideMiddle
                    }
                    ToolButton {
                        text: "×"
                        Accessible.name: qsTr("Remover pasta")
                        onClicked: dialog.controller.removeMediaFolder(
                                       mediaFolderDelegate.modelData.path)
                    }
                }
            }
            Label {
                anchors.centerIn: parent
                visible: mediaFoldersList.count === 0
                text: qsTr("Adicione uma ou mais pastas de áudio, vídeo ou imagens")
                color: "#64748b"
            }
        }
        TabBar {
            id: mediaTypeTabs
            Layout.fillWidth: true
            TabButton {
                text: qsTr("ÁUDIOS (%1)").arg(dialog.controller.folderAudioFiles.length)
            }
            TabButton {
                text: qsTr("VÍDEOS (%1)").arg(dialog.controller.folderVideoFiles.length)
            }
            TabButton {
                text: qsTr("IMAGENS (%1)").arg(dialog.controller.folderImageFiles.length)
            }
        }
        TextField {
            Layout.fillWidth: true
            placeholderText: qsTr("Pesquisar por nome de arquivo")
            text: mediaTypeTabs.currentIndex === 0 ? dialog.controller.audioFileSearch
                  : mediaTypeTabs.currentIndex === 1 ? dialog.controller.videoFileSearch
                  : dialog.controller.imageFileSearch
            onTextEdited: {
                if (mediaTypeTabs.currentIndex === 0)
                    dialog.controller.audioFileSearch = text
                else if (mediaTypeTabs.currentIndex === 1)
                    dialog.controller.videoFileSearch = text
                else
                    dialog.controller.imageFileSearch = text
            }
        }
        ListView {
            id: folderMediaList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: mediaTypeTabs.currentIndex === 0 ? dialog.controller.folderAudioFiles
                 : mediaTypeTabs.currentIndex === 1 ? dialog.controller.folderVideoFiles
                 : dialog.controller.folderImageFiles
            delegate: Rectangle {
                id: folderMediaDelegate
                required property var modelData
                width: ListView.view.width
                height: 46
                radius: 6
                color: "#142137"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 8
                    Label {
                        Layout.fillWidth: true
                        text: folderMediaDelegate.modelData.fileName
                        color: "#eff6ff"
                        elide: Text.ElideMiddle
                    }
                    Label {
                        text: folderMediaDelegate.modelData.folderPath
                        color: "#7185a3"
                        elide: Text.ElideMiddle
                        Layout.maximumWidth: 260
                    }
                    Button {
                        text: folderMediaDelegate.modelData.inPlaylist
                              ? qsTr("NA PLAYLIST") : qsTr("+ PLAYLIST")
                        enabled: !folderMediaDelegate.modelData.inPlaylist
                        onClicked: dialog.controller.addCatalogFileToPlaylist(
                                       folderMediaDelegate.modelData.path)
                    }
                }
            }
            Label {
                anchors.centerIn: parent
                visible: folderMediaList.count === 0
                text: dialog.controller.mediaFolders.length === 0
                      ? qsTr("Nenhuma pasta selecionada")
                      : qsTr("Nenhum arquivo encontrado para esta pesquisa")
                color: "#64748b"
            }
        }
    }
}
