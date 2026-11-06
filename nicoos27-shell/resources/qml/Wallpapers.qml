import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    anchors.fill: parent
    radius: 24
    color: Qt.rgba(0.04, 0.05, 0.075, 0.95)
    border.color: Qt.rgba(1, 1, 1, 0.10)
    border.width: 1
    focus: true
    Keys.onEscapePressed: surface.hideSurface()

    Connections {
        target: surface
        function onVisibleChanged() { if (surface.visible) root.forceActiveFocus() }
    }

    component PillButton: Rectangle {
        id: pb
        property string icon: ""
        property string label: ""
        signal clicked()
        Layout.preferredHeight: 34
        Layout.preferredWidth: pbRow.implicitWidth + 24
        radius: 17
        color: pbTap.pressed ? Qt.rgba(1, 1, 1, 0.22) : (pbHover.hovered ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.08))
        Behavior on color { ColorAnimation { duration: 120 } }
        RowLayout {
            id: pbRow
            anchors.centerIn: parent
            spacing: 7
            IconGlyph { source: pb.icon; iconSize: 15; visible: pb.icon.length > 0 }
            Text { text: pb.label; color: "#ffffff"; font.pixelSize: 12 }
        }
        HoverHandler { id: pbHover }
        TapHandler { id: pbTap; onTapped: pb.clicked() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            ColumnLayout {
                spacing: 2
                Text { text: "Sfondi"; color: "#ffffff"; font.pixelSize: 22; font.weight: Font.DemiBold }
                Text {
                    text: grid.count + (grid.count === 1 ? " immagine" : " immagini")
                    color: Qt.rgba(1, 1, 1, 0.5)
                    font.pixelSize: 12
                }
            }
            Item { Layout.fillWidth: true }
            PillButton { icon: "qrc:/nicoos/icons/bolt.svg"; label: "Casuale"; onClicked: wallpapers.applyRandom() }
            PillButton { label: "Aggiorna"; onClicked: wallpapers.refresh() }
            PillButton { icon: "qrc:/nicoos/icons/close.svg"; label: "Chiudi"; onClicked: surface.hideSurface() }
        }

        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: wallpaperModel
            readonly property int cols: Math.max(2, Math.floor(width / 250))
            cellWidth: width / cols
            cellHeight: cellWidth * 0.62

            delegate: Item {
                width: grid.cellWidth
                height: grid.cellHeight

                Rectangle {
                    id: card
                    anchors.fill: parent
                    anchors.margins: 8
                    radius: 16
                    clip: true
                    color: Qt.rgba(1, 1, 1, 0.05)
                    scale: tap.pressed ? 0.97 : (hover.hovered ? 1.03 : 1.0)
                    border.width: isCurrent ? 2 : 1
                    border.color: isCurrent ? accentColor : Qt.rgba(1, 1, 1, hover.hovered ? 0.3 : 0.1)
                    Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
                    Behavior on border.color { ColorAnimation { duration: 140 } }

                    Image {
                        anchors.fill: parent
                        source: wpUrl
                        sourceSize: Qt.size(480, 300)
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: false
                        opacity: status === Image.Ready ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 220 } }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 34
                        gradient: Gradient {
                            orientation: Gradient.Vertical
                            GradientStop { position: 0.0; color: "transparent" }
                            GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.75) }
                        }
                        Text {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.margins: 8
                            text: wpName
                            color: "#ffffff"
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }

                    Rectangle {
                        visible: isCurrent
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.margins: 8
                        width: 24
                        height: 24
                        radius: 12
                        color: accentColor
                        IconGlyph { anchors.centerIn: parent; source: "qrc:/nicoos/icons/check.svg"; iconSize: 14 }
                    }

                    HoverHandler { id: hover }
                    TapHandler { id: tap; onTapped: wallpapers.apply(wpPath) }
                }
            }

            Text {
                visible: grid.count === 0
                anchors.centerIn: parent
                horizontalAlignment: Text.AlignHCenter
                text: "Nessuno sfondo trovato.\nMetti le immagini in Immagini/Wallpapers\noppure in ~/.config/nicoos/wallpapers"
                color: Qt.rgba(1, 1, 1, 0.5)
                font.pixelSize: 13
                lineHeight: 1.3
            }
        }

        Text {
            visible: !wallpapers.available
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: "Per applicare gli sfondi installa swaybg (o swww)"
            color: "#ffb86b"
            font.pixelSize: 12
        }
    }
}
