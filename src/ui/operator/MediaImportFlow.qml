pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs

Item {
    id: flow

    required property var controller

    function openAudio() {
        audioDialog.open()
    }

    function openVideo() {
        videoDialog.open()
    }

    function openImage() {
        imageDialog.open()
    }

    FileDialog {
        id: audioDialog
        title: qsTr("Importar áudios")
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            qsTr("Áudios (*.mp3 *.wav *.flac *.m4a *.aac *.ogg *.opus *.wma *.aiff *.aif)")
        ]
        onAccepted: flow.controller.importAudioFiles(selectedFiles)
    }

    FileDialog {
        id: videoDialog
        title: qsTr("Importar vídeos")
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            qsTr("Vídeos (*.mp4 *.mov *.m4v *.mkv *.webm *.avi *.wmv *.mpeg *.mpg)")
        ]
        onAccepted: flow.controller.importVideoFiles(selectedFiles)
    }

    FileDialog {
        id: imageDialog
        title: qsTr("Importar imagens")
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            qsTr("Imagens (*.jpg *.jpeg *.png *.webp *.bmp *.gif *.tif *.tiff *.heic)")
        ]
        onAccepted: flow.controller.importImageFiles(selectedFiles)
    }

    DropArea {
        anchors.fill: parent
        onDropped: function(drop) {
            if (!drop.urls || drop.urls.length === 0)
                return
            const audio = []
            const video = []
            const images = []
            for (let index = 0; index < drop.urls.length; ++index) {
                const value = drop.urls[index].toString().toLowerCase()
                if (value.match(/\.(mp3|wav|flac|m4a|aac|ogg|opus|wma|aiff|aif)$/))
                    audio.push(drop.urls[index])
                else if (value.match(/\.(mp4|mov|m4v|mkv|webm|avi|wmv|mpeg|mpg)$/))
                    video.push(drop.urls[index])
                else if (value.match(/\.(jpg|jpeg|png|webp|bmp|gif|tif|tiff|heic)$/))
                    images.push(drop.urls[index])
            }
            if (audio.length > 0)
                flow.controller.importAudioFiles(audio)
            if (video.length > 0)
                flow.controller.importVideoFiles(video)
            if (images.length > 0)
                flow.controller.importImageFiles(images)
        }
    }
}
