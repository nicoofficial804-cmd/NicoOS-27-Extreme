import QtQuick
import QtQuick.Layouts

Item {
    anchors.fill: parent

    ListView {
        anchors.fill: parent
        model: notificationModel
        spacing: 8
        verticalLayoutDirection: ListView.TopToBottom
        interactive: false

        add: Transition {
            NumberAnimation { properties: "x"; from: 60; duration: animSpeedMs; easing.type: Easing.OutCubic }
            NumberAnimation { properties: "opacity"; from: 0; duration: animSpeedMs }
        }
        remove: Transition {
            NumberAnimation { properties: "opacity"; to: 0; duration: animSpeedMs }
            NumberAnimation { properties: "x"; to: 80; duration: animSpeedMs; easing.type: Easing.InCubic }
        }
        displaced: Transition {
            NumberAnimation { properties: "y"; duration: animSpeedMs; easing.type: Easing.OutCubic }
        }

        delegate: Rectangle {
            width: ListView.view.width
            height: bodyText.text.length > 0 ? 78 : 54
            radius: 14
            color: Qt.rgba(0.02, 0.02, 0.02, 0.85)
            border.color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.35)
            border.width: 1

            Rectangle {
                width: 3
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                radius: 2
                color: accentColor
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 12
                anchors.topMargin: 10
                anchors.bottomMargin: 10
                spacing: 2

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: appName
                        color: accentColor
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                        Layout.fillWidth: true
                    }
                    Text {
                        text: "\u2715"
                        color: Qt.rgba(0.89, 0.15, 0.17, 0.4)
                        font.pixelSize: 11
                        MouseArea {
                            anchors.fill: parent
                            anchors.margins: -6
                            onClicked: notificationModel.dismiss(notifId)
                        }
                    }
                }
                Text {
                    text: summary
                    color: "#e3272b"
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                Text {
                    id: bodyText
                    text: body
                    color: Qt.rgba(0.89, 0.15, 0.17, 0.55)
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }
    }
}
