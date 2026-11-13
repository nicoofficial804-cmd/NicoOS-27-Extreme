import QtQuick
import QtQuick.Layouts

// Riquadro di attivazione rapida per il centro di controllo.
Rectangle {
    id: tile
    property string icon: ""
    property string label: ""
    property bool active: false
    property bool tileEnabled: true
    signal clicked()

    Layout.fillWidth: true
    Layout.preferredWidth: 1   // colonne di uguale larghezza in una GridLayout
    Layout.preferredHeight: 66
    radius: 16
    opacity: tileEnabled ? 1.0 : 0.38
    scale: tap.pressed ? 0.94 : (hover.hovered && tileEnabled ? 1.03 : 1.0)

    color: active ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.30)
                  : (hover.hovered && tileEnabled ? Qt.rgba(0.89, 0.15, 0.17, 0.11) : Qt.rgba(0.89, 0.15, 0.17, 0.06))
    border.width: 1
    border.color: active ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.75)
                         : Qt.rgba(0.89, 0.15, 0.17, 0.09)

    Behavior on color { ColorAnimation { duration: 140 } }
    Behavior on border.color { ColorAnimation { duration: 140 } }
    Behavior on scale { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }
    Behavior on opacity { NumberAnimation { duration: 140 } }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 5
        IconGlyph {
            Layout.alignment: Qt.AlignHCenter
            source: tile.icon
            iconSize: 18
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: tile.label
            color: "#e3272b"
            font.pixelSize: 11
            font.weight: tile.active ? Font.DemiBold : Font.Normal
        }
    }

    HoverHandler { id: hover }
    TapHandler {
        id: tap
        enabled: tile.tileEnabled
        onTapped: tile.clicked()
    }
}
