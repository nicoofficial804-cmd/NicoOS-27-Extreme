import QtQuick
import QtQuick.Layouts

// Calendario mensile (settimane che iniziano di lunedi') con una grande
// intestazione con l'orologio.
Rectangle {
    id: root
    anchors.fill: parent
    radius: 22
    color: Qt.rgba(0.02, 0.02, 0.02, 0.94)
    border.color: Qt.rgba(0.89, 0.15, 0.17, 0.10)
    border.width: 1
    focus: true
    Keys.onEscapePressed: surface.hideSurface()

    property date now: new Date()
    property int viewYear: now.getFullYear()
    property int viewMonth: now.getMonth()

    Timer { interval: 1000; running: surface.visible; repeat: true; onTriggered: root.now = new Date() }

    Connections {
        target: surface
        function onVisibleChanged() {
            if (surface.visible) {
                root.now = new Date()
                root.viewYear = root.now.getFullYear()
                root.viewMonth = root.now.getMonth()
            }
        }
    }

    function shiftMonth(delta) {
        var m = viewMonth + delta
        var y = viewYear
        while (m < 0) { m += 12; y -= 1 }
        while (m > 11) { m -= 12; y += 1 }
        viewMonth = m
        viewYear = y
    }

    readonly property int firstOffset: (new Date(viewYear, viewMonth, 1).getDay() + 6) % 7   // lunedi' = 0
    readonly property int daysInMonth: new Date(viewYear, viewMonth + 1, 0).getDate()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        ColumnLayout {
            spacing: 0
            Text {
                text: Qt.formatDateTime(root.now, "HH:mm:ss")
                color: "#e3272b"
                font.pixelSize: 36
                font.weight: Font.Light
            }
            Text {
                text: Qt.locale().dayName(root.now.getDay(), Locale.LongFormat) + " " +
                      root.now.getDate() + " " + Qt.locale().monthName(root.now.getMonth(), Locale.LongFormat)
                color: Qt.rgba(0.89, 0.15, 0.17, 0.6)
                font.pixelSize: 13
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: Qt.locale().monthName(root.viewMonth, Locale.LongFormat) + " " + root.viewYear
                color: "#e3272b"
                font.pixelSize: 15
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            Repeater {
                model: [-1, 1]
                delegate: Rectangle {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    radius: 14
                    color: navTap.pressed ? Qt.rgba(0.89, 0.15, 0.17, 0.22) : (navHover.hovered ? Qt.rgba(0.89, 0.15, 0.17, 0.13) : Qt.rgba(0.89, 0.15, 0.17, 0.06))
                    Behavior on color { ColorAnimation { duration: 110 } }
                    IconGlyph {
                        anchors.centerIn: parent
                        source: "qrc:/nicoos/icons/chevron-down.svg"
                        iconSize: 14
                        rotation: modelData < 0 ? 90 : -90
                    }
                    HoverHandler { id: navHover }
                    TapHandler { id: navTap; onTapped: root.shiftMonth(modelData) }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 7
            rowSpacing: 4
            columnSpacing: 0

            Repeater {
                model: 7
                delegate: Text {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    horizontalAlignment: Text.AlignHCenter
                    text: Qt.locale().dayName((index + 1) % 7, Locale.ShortFormat).substring(0, 3)
                    color: Qt.rgba(0.89, 0.15, 0.17, 0.45)
                    font.pixelSize: 11
                }
            }

            Repeater {
                model: 42
                delegate: Item {
                    readonly property int dayNum: index - root.firstOffset + 1
                    readonly property bool inMonth: dayNum >= 1 && dayNum <= root.daysInMonth
                    readonly property bool isToday: inMonth && dayNum === root.now.getDate()
                        && root.viewMonth === root.now.getMonth() && root.viewYear === root.now.getFullYear()

                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 34

                    Rectangle {
                        anchors.centerIn: parent
                        width: 30
                        height: 30
                        radius: 15
                        color: isToday ? accentColor : "transparent"
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: inMonth
                        text: dayNum
                        color: isToday ? "#000000" : "#e3272b"
                        font.pixelSize: 13
                        font.weight: isToday ? Font.DemiBold : Font.Normal
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
