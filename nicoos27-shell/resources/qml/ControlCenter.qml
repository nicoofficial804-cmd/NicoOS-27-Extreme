import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

Rectangle {
    id: root
    anchors.fill: parent
    radius: 22
    color: Qt.rgba(0.045, 0.06, 0.085, 0.93)
    border.color: Qt.rgba(1, 1, 1, 0.10)
    border.width: 1
    focus: true
    Keys.onEscapePressed: surface.hideSurface()

    function profileName(p) {
        if (p === "performance") return "Prestazioni"
        if (p === "power-saver") return "Risparmio"
        if (p === "balanced") return "Bilanciato"
        return "Profilo"
    }

    // ---- Elementi riutilizzabili -------------------------------------
    component RoundBtn: Rectangle {
        id: rb
        property string icon: ""
        property int size: 34
        property bool strong: false
        signal clicked()
        width: size
        height: size
        radius: size / 2
        color: rbTap.pressed ? Qt.rgba(1, 1, 1, 0.26)
                             : (rbHover.hovered ? Qt.rgba(1, 1, 1, 0.17) : Qt.rgba(1, 1, 1, strong ? 0.15 : 0.07))
        Behavior on color { ColorAnimation { duration: 120 } }
        IconGlyph { anchors.centerIn: parent; source: rb.icon; iconSize: Math.round(rb.size * 0.46) }
        HoverHandler { id: rbHover }
        TapHandler { id: rbTap; onTapped: rb.clicked() }
    }

    component GlassSlider: Slider {
        id: s
        implicitHeight: 22
        background: Rectangle {
            x: s.leftPadding
            y: s.topPadding + s.availableHeight / 2 - height / 2
            width: s.availableWidth
            height: 6
            radius: 3
            color: Qt.rgba(1, 1, 1, 0.14)
            Rectangle {
                width: s.visualPosition * parent.width
                height: parent.height
                radius: 3
                color: accentColor
            }
        }
        handle: Rectangle {
            x: s.leftPadding + s.visualPosition * (s.availableWidth - width)
            y: s.topPadding + s.availableHeight / 2 - height / 2
            width: s.pressed ? 20 : 16
            height: width
            radius: width / 2
            color: "#ffffff"
            Behavior on width { NumberAnimation { duration: 90 } }
        }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        anchors.margins: 16
        contentWidth: width
        contentHeight: col.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: col
            width: flick.width
            spacing: 12

            // ---- Intestazione ---------------------------------------
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "Centro di controllo"
                    color: "#ffffff"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                }
                RoundBtn { icon: "qrc:/nicoos/icons/close.svg"; size: 28; onClicked: surface.hideSurface() }
            }

            // ---- Media ----------------------------------------------
            Rectangle {
                visible: media.hasPlayer
                Layout.fillWidth: true
                Layout.preferredHeight: 78
                radius: 16
                color: Qt.rgba(1, 1, 1, 0.06)

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            Layout.fillWidth: true
                            text: media.title.length > 0 ? media.title : "In riproduzione"
                            color: "#ffffff"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: media.artist
                            color: Qt.rgba(1, 1, 1, 0.6)
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }
                    RowLayout {
                        spacing: 6
                        RoundBtn { icon: "qrc:/nicoos/icons/prev.svg"; onClicked: media.previous() }
                        RoundBtn {
                            icon: media.playing ? "qrc:/nicoos/icons/pause.svg" : "qrc:/nicoos/icons/play.svg"
                            size: 40
                            strong: true
                            onClicked: media.playPause()
                        }
                        RoundBtn { icon: "qrc:/nicoos/icons/next.svg"; onClicked: media.next() }
                    }
                }
            }

            // ---- Riquadri rapidi ------------------------------------
            GridLayout {
                Layout.fillWidth: true
                columns: 3
                rowSpacing: 8
                columnSpacing: 8

                Tile {
                    icon: "qrc:/nicoos/icons/bell-off.svg"
                    label: "Non disturbare"
                    active: quick.doNotDisturb
                    onClicked: quick.toggleDoNotDisturb()
                }
                Tile {
                    icon: "qrc:/nicoos/icons/moon.svg"
                    label: "Luce notturna"
                    active: quick.nightLight
                    tileEnabled: quick.hasNightLight
                    onClicked: quick.toggleNightLight()
                }
                Tile {
                    icon: "qrc:/nicoos/icons/coffee.svg"
                    label: "Caffeina"
                    active: quick.caffeine
                    tileEnabled: quick.hasCaffeine
                    onClicked: quick.toggleCaffeine()
                }
                Tile {
                    icon: "qrc:/nicoos/icons/bolt.svg"
                    label: root.profileName(quick.powerProfile)
                    active: quick.powerProfile === "performance"
                    tileEnabled: quick.hasProfiles
                    onClicked: quick.cycleProfile()
                }
                Tile {
                    icon: "qrc:/nicoos/icons/record.svg"
                    label: quick.recording ? "Ferma" : "Registra"
                    active: quick.recording
                    tileEnabled: quick.hasRecorder
                    onClicked: quick.toggleRecording()
                }
                Tile {
                    icon: "qrc:/nicoos/icons/area.svg"
                    label: "Area schermo"
                    tileEnabled: quick.hasScreenshot
                    onClicked: quick.screenshot(true)
                }
                Tile {
                    icon: "qrc:/nicoos/icons/screen.svg"
                    label: "Schermo intero"
                    tileEnabled: quick.hasScreenshot
                    onClicked: quick.screenshot(false)
                }
                Tile {
                    icon: "qrc:/nicoos/icons/image.svg"
                    label: "Sfondi"
                    onClicked: shell.toggle("wallpapers")
                }
                Tile {
                    icon: "qrc:/nicoos/icons/clipboard.svg"
                    label: "Appunti"
                    onClicked: shell.toggle("clipboard")
                }
            }

            // ---- Rete (si espande nell'elenco Wi-Fi) -----------------
            Rectangle {
                id: netCard
                property bool expanded: false
                property string pendingSsid: ""
                readonly property real listH: Math.min(6, wifi.count) * 42
                property real cardH: expanded
                    ? 62 + listH + (wifi.count === 0 ? 40 : 0) + (pendingSsid.length > 0 ? 52 : 0) + 8
                    : 62
                Behavior on cardH { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }

                Layout.fillWidth: true
                Layout.preferredHeight: cardH
                radius: 16
                color: Qt.rgba(1, 1, 1, 0.06)
                clip: true

                Item {
                    id: netHead
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 62

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 12
                        IconGlyph {
                            source: networkClient.connected ? "qrc:/nicoos/icons/network-wifi.svg"
                                                            : "qrc:/nicoos/icons/network-off.svg"
                            iconSize: 20
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                Layout.fillWidth: true
                                text: networkClient.connected ? networkClient.ssid : "Non connesso"
                                color: "#ffffff"
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }
                            Text {
                                text: networkClient.kindLabel
                                color: Qt.rgba(1, 1, 1, 0.55)
                                font.pixelSize: 11
                            }
                        }
                        IconGlyph {
                            source: "qrc:/nicoos/icons/chevron-down.svg"
                            iconSize: 16
                            rotation: netCard.expanded ? 180 : 0
                            Behavior on rotation { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
                        }
                    }
                    TapHandler {
                        onTapped: {
                            netCard.expanded = !netCard.expanded
                            if (netCard.expanded) wifi.scan()
                        }
                    }
                }

                ListView {
                    id: wifiList
                    anchors.top: netHead.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    height: netCard.listH
                    visible: netCard.expanded
                    interactive: false
                    clip: true
                    model: wifi

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 40
                        radius: 10
                        color: wifiHover.hovered ? Qt.rgba(1, 1, 1, 0.09) : "transparent"
                        Behavior on color { ColorAnimation { duration: 110 } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 10
                            IconGlyph {
                                source: secure ? "qrc:/nicoos/icons/wifi-lock.svg" : "qrc:/nicoos/icons/network-wifi.svg"
                                iconSize: 16
                                opacity: 0.4 + (strength / 100) * 0.6
                            }
                            Text {
                                Layout.fillWidth: true
                                text: ssid
                                color: "#ffffff"
                                font.pixelSize: 13
                                font.weight: active ? Font.DemiBold : Font.Normal
                                elide: Text.ElideRight
                            }
                            IconGlyph {
                                visible: active
                                source: "qrc:/nicoos/icons/check.svg"
                                tint: accentColor
                                iconSize: 14
                            }
                        }
                        HoverHandler { id: wifiHover }
                        TapHandler {
                            onTapped: {
                                if (!active) {
                                    netCard.pendingSsid = ""
                                    wifi.connectTo(ssid)
                                }
                            }
                        }
                    }
                }

                Text {
                    visible: netCard.expanded && wifi.count === 0
                    anchors.top: netHead.bottom
                    anchors.topMargin: 8
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: !wifi.available ? "nmcli non disponibile"
                                          : (wifi.busy ? "Ricerca reti…" : "Nessuna rete trovata")
                    color: Qt.rgba(1, 1, 1, 0.5)
                    font.pixelSize: 12
                }

                Rectangle {
                    id: pwBox
                    visible: netCard.expanded && netCard.pendingSsid.length > 0
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 8
                    height: 40
                    radius: 10
                    color: Qt.rgba(1, 1, 1, 0.09)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 6
                        spacing: 8
                        TextInput {
                            id: pwInput
                            Layout.fillWidth: true
                            color: "#ffffff"
                            font.pixelSize: 13
                            echoMode: TextInput.Password
                            verticalAlignment: TextInput.AlignVCenter
                            onAccepted: {
                                wifi.connectWithPassword(netCard.pendingSsid, text)
                                text = ""
                                netCard.pendingSsid = ""
                            }
                            Text {
                                visible: pwInput.text.length === 0
                                anchors.verticalCenter: parent.verticalCenter
                                text: "Password per " + netCard.pendingSsid
                                color: Qt.rgba(1, 1, 1, 0.4)
                                font.pixelSize: 13
                            }
                        }
                        RoundBtn { icon: "qrc:/nicoos/icons/check.svg"; size: 28; onClicked: pwInput.accepted() }
                    }
                }

                Connections {
                    target: wifi
                    function onPasswordRequired(ssid) {
                        netCard.pendingSsid = ssid
                        pwInput.forceActiveFocus()
                    }
                }
            }

            // ---- Volume ------------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 76
                radius: 16
                color: Qt.rgba(1, 1, 1, 0.06)

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 6
                    RowLayout {
                        spacing: 10
                        RoundBtn {
                            icon: audioClient.muted ? "qrc:/nicoos/icons/audio-muted.svg" : "qrc:/nicoos/icons/audio-on.svg"
                            size: 28
                            onClicked: audioClient.toggleMute()
                        }
                        Text { text: "Volume"; color: "#ffffff"; font.pixelSize: 13; Layout.fillWidth: true }
                        Text {
                            text: audioClient.muted ? "Muto" : audioClient.volume + "%"
                            color: accentColor
                            font.pixelSize: 12
                        }
                    }
                    GlassSlider {
                        id: volumeSlider
                        objectName: "volumeSlider"
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        onMoved: audioClient.setVolume(Math.round(value))
                    }
                    // Segue il volume reale, ma non combatte mai con il dito
                    // dell'utente.
                    Binding {
                        target: volumeSlider
                        property: "value"
                        value: audioClient.volume
                        when: !volumeSlider.pressed
                    }
                }
            }

            // ---- Luminosita' -----------------------------------------
            Rectangle {
                visible: brightnessClient.available
                Layout.fillWidth: true
                Layout.preferredHeight: 76
                radius: 16
                color: Qt.rgba(1, 1, 1, 0.06)

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 6
                    RowLayout {
                        spacing: 10
                        IconGlyph { source: "qrc:/nicoos/icons/brightness.svg"; iconSize: 18 }
                        Text { text: "Luminosità"; color: "#ffffff"; font.pixelSize: 13; Layout.fillWidth: true }
                        Text { text: brightnessClient.brightnessPercent + "%"; color: accentColor; font.pixelSize: 12 }
                    }
                    GlassSlider {
                        id: brightnessSlider
                        objectName: "brightnessSlider"
                        Layout.fillWidth: true
                        from: 1
                        to: 100
                        onMoved: brightnessClient.setBrightnessPercent(Math.round(value))
                    }
                    Binding {
                        target: brightnessSlider
                        property: "value"
                        value: brightnessClient.brightnessPercent
                        when: !brightnessSlider.pressed
                    }
                }
            }

            // ---- Bluetooth -------------------------------------------
            Rectangle {
                visible: bluetoothClient.available
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                radius: 16
                color: Qt.rgba(1, 1, 1, 0.06)

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 12
                    IconGlyph { source: "qrc:/nicoos/icons/bluetooth.svg"; iconSize: 18; opacity: bluetoothClient.powered ? 1.0 : 0.45 }
                    Text { text: "Bluetooth"; color: "#ffffff"; font.pixelSize: 13; Layout.fillWidth: true }
                    Switch {
                        checked: bluetoothClient.powered
                        onToggled: bluetoothClient.setPowered(checked)
                    }
                }
            }

            // ---- Sessione --------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Tile {
                    Layout.preferredHeight: 58
                    icon: "qrc:/nicoos/icons/lock.svg"
                    label: "Blocca"
                    onClicked: { surface.hideSurface(); powerActions.trigger("lock") }
                }
                Tile {
                    Layout.preferredHeight: 58
                    icon: "qrc:/nicoos/icons/sleep.svg"
                    label: "Sospendi"
                    onClicked: { surface.hideSurface(); powerActions.trigger("suspend") }
                }
                Tile {
                    Layout.preferredHeight: 58
                    icon: "qrc:/nicoos/icons/power.svg"
                    label: "Alimentazione"
                    onClicked: shell.toggle("power")
                }
            }

            Item { Layout.preferredHeight: 4 }
        }
    }
}
