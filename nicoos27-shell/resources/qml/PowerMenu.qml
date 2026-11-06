import QtQuick
import QtQuick.Layouts

// Menu di spegnimento a tutto schermo. Le azioni distruttive (esci,
// riavvia, spegni) richiedono un secondo clic/Invio entro 3 s; sospendi e
// blocca vengono eseguiti subito.
Rectangle {
    id: root
    anchors.fill: parent
    color: Qt.rgba(0.01, 0.02, 0.04, 0.86)
    focus: true
    opacity: 0

    property int selected: -1
    property string armed: ""

    readonly property var actions: [
        { id: "suspend",  label: "Sospendi", icon: "qrc:/nicoos/icons/sleep.svg",  confirm: false },
        { id: "lock",     label: "Blocca",   icon: "qrc:/nicoos/icons/lock.svg",   confirm: false },
        { id: "logout",   label: "Esci",     icon: "qrc:/nicoos/icons/logout.svg", confirm: true },
        { id: "reboot",   label: "Riavvia",  icon: "qrc:/nicoos/icons/reboot.svg", confirm: true },
        { id: "poweroff", label: "Spegni",   icon: "qrc:/nicoos/icons/power.svg",  confirm: true }
    ]

    function trigger(i) {
        if (i < 0 || i >= actions.length) return
        var a = actions[i]
        if (a.confirm && armed !== a.id) {
            armed = a.id
            armTimer.restart()
            return
        }
        surface.hideSurface()
        powerActions.trigger(a.id)
    }

    Keys.onEscapePressed: surface.hideSurface()
    Keys.onLeftPressed: selected = selected <= 0 ? actions.length - 1 : selected - 1
    Keys.onRightPressed: selected = (selected + 1) % actions.length
    Keys.onReturnPressed: trigger(selected)
    Keys.onEnterPressed: trigger(selected)
    Keys.onPressed: function(event) {
        if (event.key >= Qt.Key_1 && event.key <= Qt.Key_5) {
            trigger(event.key - Qt.Key_1)
            event.accepted = true
        }
    }

    Connections {
        target: surface
        function onVisibleChanged() {
            if (surface.visible) {
                root.armed = ""
                root.selected = -1
                fadeIn.restart()
                root.forceActiveFocus()
            } else {
                root.opacity = 0
            }
        }
    }

    NumberAnimation { id: fadeIn; target: root; property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.OutCubic }
    Timer { id: armTimer; interval: 3000; onTriggered: root.armed = "" }

    // un clic sull'area vuota chiude il menu
    MouseArea { anchors.fill: parent; z: -1; onClicked: surface.hideSurface() }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 34

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Cosa vuoi fare?"
            color: "#ffffff"
            font.pixelSize: 26
            font.weight: Font.Light
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 22

            Repeater {
                model: root.actions
                delegate: Rectangle {
                    id: card
                    readonly property bool isArmed: root.armed === modelData.id
                    readonly property bool isSel: root.selected === index

                    Layout.preferredWidth: 136
                    Layout.preferredHeight: 150
                    radius: 28
                    scale: cardTap.pressed ? 0.94 : ((isSel || cardHover.hovered) ? 1.07 : 1.0)
                    color: isArmed ? Qt.rgba(1, 0.28, 0.28, 0.40)
                                   : (isSel || cardHover.hovered ? Qt.rgba(1, 1, 1, 0.17) : Qt.rgba(1, 1, 1, 0.07))
                    border.width: 1
                    border.color: isArmed ? Qt.rgba(1, 0.4, 0.4, 0.9)
                                          : (isSel || cardHover.hovered ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.8)
                                                                        : Qt.rgba(1, 1, 1, 0.10))

                    Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
                    Behavior on color { ColorAnimation { duration: 140 } }
                    Behavior on border.color { ColorAnimation { duration: 140 } }

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 14
                        IconGlyph {
                            Layout.alignment: Qt.AlignHCenter
                            source: modelData.icon
                            iconSize: 36
                        }
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: card.isArmed ? "Conferma?" : modelData.label
                            color: "#ffffff"
                            font.pixelSize: 15
                            font.weight: card.isArmed ? Font.DemiBold : Font.Normal
                        }
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: String(index + 1)
                            color: Qt.rgba(1, 1, 1, 0.35)
                            font.pixelSize: 11
                        }
                    }

                    HoverHandler {
                        id: cardHover
                        onHoveredChanged: if (hovered) root.selected = index
                    }
                    TapHandler { id: cardTap; onTapped: root.trigger(index) }
                }
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Esc per annullare   ·   ← → e Invio, oppure 1–5"
            color: Qt.rgba(1, 1, 1, 0.4)
            font.pixelSize: 12
        }
    }
}
