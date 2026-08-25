import QtQuick
import QtQuick.Controls

Rectangle {
    id: thumbnail

    property url source
    property string mediaType: "audio"
    property string mediaPath
    required property var controller
    property color accentColor: "#8bbcff"

    function requestThumbnail() {
        if (thumbnail.controller
                && thumbnail.source.toString().length === 0
                && thumbnail.mediaPath.length > 0
                && thumbnail.mediaType !== "image"
                && thumbnail.mediaType !== "lyrics") {
            thumbnail.controller.requestMediaThumbnail(thumbnail.mediaPath,
                                                        thumbnail.mediaType)
        }
    }

    Component.onCompleted: requestThumbnail()
    onMediaPathChanged: requestThumbnail()
    onMediaTypeChanged: requestThumbnail()

    implicitWidth: 68
    implicitHeight: 40
    color: "#0f1215"
    border.color: "#46515b"
    border.width: 1
    radius: 3
    clip: true
    Accessible.ignored: true

    Image {
        id: thumbnailImage
        anchors.fill: parent
        source: thumbnail.source
        visible: status === Image.Ready
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: true
    }

    Label {
        anchors.centerIn: parent
        Accessible.ignored: true
        visible: thumbnailImage.status !== Image.Ready
        text: thumbnail.mediaType === "video" ? "▶"
              : thumbnail.mediaType === "image" ? "▧"
              : thumbnail.mediaType === "lyrics" ? "≡" : "♫"
        color: thumbnail.accentColor
        font.pixelSize: UiScale.px(17)
    }
}
