import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    anchors.fill: parent
    radius: 22
    color: Qt.rgba(0.02, 0.02, 0.02, 0.95)
    border.color: Qt.rgba(0.89, 0.15, 0.17, 0.10)
    border.width: 1
    focus: true
    Keys.onEscapePressed: surface.hideSurface()

    Connections {
        target: surface
        function onVisibleChanged() {
            if (surface.visible) {
                search.text = ""
                search.forceActiveFocus()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "Cronologia appunti"
                color: "#e3272b"
                font.pixelSize: 18
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            Rectangle {
                visible: clipboard.available && clipboard.count > 0
                Layout.preferredHeight: 30
                Layout.preferredWidth: wipeRow.implicitWidth + 20
                radius: 15
                color: wipeTap.pressed ? Qt.rgba(1, 0.3, 0.3, 0.4) : (wipeHover.hovered ? Qt.rgba(1, 0.3, 0.3, 0.25) : Qt.rgba(0.89, 0.15, 0.17, 0.08))
                Behavior on color { ColorAnimation { duration: 120 } }
                RowLayout {
                    id: wipeRow
                    anchors.centerIn: parent
                    spacing: 6
                    IconGlyph { source: "qrc:/nicoos/icons/trash.svg"; iconSize: 14 }
                    Text { text: "Svuota"; color: "#e3272b"; font.pixelSize: 12 }
                }
                HoverHandler { id: wipeHover }
                TapHandler { id: wipeTap; onTapped: clipboard.wipe() }
            }
        }

        Rectangle {
            visible: clipboard.available
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            radius: 12
            color: Qt.rgba(0.89, 0.15, 0.17, 0.07)
            border.width: 1
            border.color: search.activeFocus ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.7) : Qt.rgba(0.89, 0.15, 0.17, 0.08)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8
                IconGlyph { source: "qrc:/nicoos/icons/search.svg"; iconSize: 15; opacity: 0.6 }
                TextInput {
                    id: search
                    Layout.fillWidth: true
                    color: "#e3272b"
                    font.pixelSize: 14
                    verticalAlignment: TextInput.AlignVCenter
                    onTextChanged: clipboard.filterText = text
                    Keys.onDownPressed: list.forceActiveFocus()
                    Keys.onReturnPressed: if (list.count > 0) { clipboard.select(0); surface.hideSurface() }
                    Text {
                        visible: search.text.length === 0
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Cerca negli appunti…"
                        color: Qt.rgba(0.89, 0.15, 0.17, 0.4)
                        font.pixelSize: 14
                    }
                }
            }
        }

        ListView {
            id: list
            visible: clipboard.available
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: clipboard
            currentIndex: 0
            highlightFollowsCurrentItem: true
            keyNavigationEnabled: true
            Keys.onReturnPressed: { clipboard.select(currentIndex); surface.hideSurface() }
            Keys.onUpPressed: if (currentIndex === 0) search.forceActiveFocus(); else decrementCurrentIndex()

            add: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 160 } }
            displaced: Transition { NumberAnimation { properties: "y"; duration: 180; easing.type: Easing.OutCubic } }

            delegate: Rectangle {
                width: ListView.view.width
                height: 44
                radius: 11
                color: rowTap.pressed ? Qt.rgba(0.89, 0.15, 0.17, 0.18)
                                      : (rowHover.hovered || ListView.isCurrentItem ? Qt.rgba(0.89, 0.15, 0.17, 0.10) : Qt.rgba(0.89, 0.15, 0.17, 0.04))
                Behavior on color { ColorAnimation { duration: 110 } }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 8
                    spacing: 10
                    IconGlyph {
                        source: clipIsImage ? "qrc:/nicoos/icons/image.svg" : "qrc:/nicoos/icons/clipboard.svg"
                        iconSize: 16
                        opacity: 0.7
                    }
                    Text {
                        Layout.fillWidth: true
                        text: clipText
                        color: "#e3272b"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }
                    Rectangle {
                        visible: rowHover.hovered
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 26
                        radius: 13
                        color: delHover.hovered ? Qt.rgba(1, 0.3, 0.3, 0.45) : "transparent"
                        IconGlyph { anchors.centerIn: parent; source: "qrc:/nicoos/icons/trash.svg"; iconSize: 14 }
                        HoverHandler { id: delHover }
                        TapHandler { onTapped: clipboard.remove(index) }
                    }
                }
                HoverHandler { id: rowHover }
                TapHandler {
                    id: rowTap
                    onTapped: { clipboard.select(index); surface.hideSurface() }
                }
            }

            Text {
                visible: list.count === 0
                anchors.centerIn: parent
                text: "Nessun elemento copiato"
                color: Qt.rgba(0.89, 0.15, 0.17, 0.45)
                font.pixelSize: 13
            }
        }

        Text {
            visible: !clipboard.available
            Layout.fillWidth: true
            Layout.fillHeight: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            text: "Per usare la cronologia appunti installa\ncliphist e wl-clipboard"
            color: "#ffb86b"
            font.pixelSize: 13
        }
    }
}
