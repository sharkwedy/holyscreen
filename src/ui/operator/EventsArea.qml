pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: area
    required property var context
    required property var sourceController
    signal clearHistoryRequested()
    spacing: 8

    function duration(milliseconds) {
        const totalSeconds = Math.max(0, Math.floor((milliseconds || 0) / 1000))
        const hours = Math.floor(totalSeconds / 3600)
        const minutes = Math.floor((totalSeconds % 3600) / 60)
        const seconds = totalSeconds % 60
        return (hours > 0 ? hours + ":" : "")
                + (hours > 0 && minutes < 10 ? "0" : "") + minutes
                + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
    Label { text: "PLAYLIST DE CULTO"; color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
    RowLayout {
        Layout.fillWidth: true
        TextField { id: newEventTitle; Layout.fillWidth: true; placeholderText: "Nome do culto"; Accessible.name: placeholderText }
        TextField { id: newEventDate; Layout.preferredWidth: 120; placeholderText: "Data/hora"; Accessible.name: placeholderText }
        Button {
            text: "NOVO"
            Accessible.name: "Criar playlist de culto"
            onClicked: {
                if (newEventTitle.text.trim().length === 0) return
                area.context.createEvent(newEventTitle.text, newEventDate.text)
                newEventTitle.clear()
                newEventDate.clear()
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        ComboBox {
            id: eventPicker
            Layout.fillWidth: true
            model: area.context.events
            textRole: "title"
            valueRole: "id"
            Accessible.name: "Playlist de culto atual"
            onActivated: area.context.selectEvent(currentValue)
        }
        Button { text: "ABRIR"; enabled: eventPicker.currentValue !== undefined; onClicked: area.context.selectEvent(eventPicker.currentValue) }
        Button { text: "EXCLUIR"; enabled: area.context.currentEventId.length > 0; onClicked: area.context.deleteEvent(area.context.currentEventId) }
    }
    Label { text: "Duração total: " + area.duration(area.context.eventDurationMs); color: "#c8d5e8" }
    Flow {
        Layout.fillWidth: true
        spacing: 5
        Button {
            text: "+ APRESENTAÇÃO"
            enabled: area.context.currentEventId.length > 0 && area.sourceController.currentPresentationId.length > 0
            onClicked: area.context.addEventItem(area.sourceController.currentPresentationType,
                                                  area.sourceController.currentPresentationId,
                                                  area.sourceController.currentPresentationTitle, 0)
        }
        Button {
            text: "+ IMAGEM"
            enabled: area.context.currentEventId.length > 0 && area.sourceController.currentImageId.length > 0
            onClicked: area.context.addEventItem("image", area.sourceController.currentImageId,
                                                  area.sourceController.currentImageTitle, 0)
        }
        Button {
            text: "+ VÍDEO"
            enabled: area.context.currentEventId.length > 0 && area.sourceController.currentVideoId.length > 0
            onClicked: area.context.addEventItem("video", area.sourceController.currentVideoId,
                                                  area.sourceController.currentVideoTitle,
                                                  area.sourceController.videoDurationMs)
        }
        Button {
            text: "+ ÁUDIO"
            enabled: area.context.currentEventId.length > 0 && area.sourceController.currentAudioId.length > 0
            onClicked: area.context.addEventItem("audio", area.sourceController.currentAudioId,
                                                  area.sourceController.currentAudioTitle,
                                                  area.sourceController.audioDurationMs)
        }
    }
    ListView {
        id: eventItemsList
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(260, Math.max(60, contentHeight))
        clip: true
        spacing: 4
        model: area.context.eventItems
        delegate: Rectangle {
            id: eventItemDelegate
            required property var modelData
            required property int index
            width: ListView.view.width
            height: 46
            color: "#142137"
            radius: 6
            RowLayout {
                anchors.fill: parent
                anchors.margins: 5
                Label {
                    text: "⋮⋮"
                    color: "#8da0bc"
                    Accessible.name: "Arrastar para reordenar " + eventItemDelegate.modelData.title
                    DragHandler {
                        target: null
                        onActiveChanged: {
                            if (active) return
                            const targetIndex = Math.max(0, Math.min(eventItemsList.count - 1,
                                eventItemDelegate.index + Math.round(translation.y / eventItemDelegate.height)))
                            if (targetIndex !== eventItemDelegate.index)
                                area.context.moveEventItem(eventItemDelegate.modelData.id, targetIndex)
                        }
                    }
                }
                Label { text: eventItemDelegate.modelData.type.toUpperCase(); color: "#70e1a7"; font.pixelSize: 9 }
                Label { Layout.fillWidth: true; text: eventItemDelegate.modelData.title; color: "#eff6ff"; elide: Text.ElideRight }
                Label { text: area.duration(eventItemDelegate.modelData.durationMs); color: "#8da0bc"; font.pixelSize: 10 }
                ToolButton { text: "↑"; Accessible.name: "Mover para cima"; enabled: eventItemDelegate.index > 0; onClicked: area.context.moveEventItem(eventItemDelegate.modelData.id, eventItemDelegate.index - 1) }
                ToolButton { text: "↓"; Accessible.name: "Mover para baixo"; enabled: eventItemDelegate.index + 1 < eventItemsList.count; onClicked: area.context.moveEventItem(eventItemDelegate.modelData.id, eventItemDelegate.index + 1) }
                Button { text: "EXECUTAR"; onClicked: area.context.executeEventItem(eventItemDelegate.modelData.id) }
                ToolButton { text: "×"; Accessible.name: "Remover item"; onClicked: area.context.removeEventItem(eventItemDelegate.modelData.id) }
            }
        }
    }

    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#24334b" }
    RowLayout {
        Layout.fillWidth: true
        Label { text: "HISTÓRICO"; color: "#8da0bc"; font.bold: true; font.pixelSize: 11 }
        Item { Layout.fillWidth: true }
        Button { text: "LIMPAR"; enabled: area.context.history.length > 0; onClicked: area.clearHistoryRequested() }
    }
    Label {
        Layout.fillWidth: true
        text: "Execuções: " + (area.context.historyReport.totalExecutions || 0)
              + "  •  Mais executado: " + (area.context.historyReport.mostExecutedTitle || "—")
        color: "#c8d5e8"
        wrapMode: Text.WordWrap
    }
    ListView {
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(220, Math.max(50, contentHeight))
        clip: true
        spacing: 3
        model: area.context.history
        delegate: Rectangle {
            id: historyDelegate
            required property var modelData
            width: ListView.view.width
            height: 40
            color: "#142137"
            radius: 5
            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                Label { text: historyDelegate.modelData.type.toUpperCase(); color: "#70e1a7"; font.pixelSize: 9 }
                Label { Layout.fillWidth: true; text: historyDelegate.modelData.title; color: "#eff6ff"; elide: Text.ElideRight }
                Label { text: historyDelegate.modelData.executedAt.replace("T", " ").slice(0, 16); color: "#8da0bc"; font.pixelSize: 9 }
            }
        }
    }
}
