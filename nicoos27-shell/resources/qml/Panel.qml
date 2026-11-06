import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    color: "transparent"

    readonly property int barH: panelHeightPx
    readonly property int iconPx: Math.max(14, Math.min(20, barH - 18))
    property date now: new Date()

    // Aggiorna ogni secondo solo se il formato dell'orologio mostra davvero
    // i secondi; altrimenti si risveglia una volta al minuto, allineato al
    // cambio di minuto.
    readonly property bool showSeconds: clockFormat.indexOf("s") >= 0
    Timer {
        id: clockTimer
        interval: root.showSeconds ? 1000 : 60000
        running: true
        repeat: true
        triggeredOnStart: false
        onTriggered: {
            root.now = new Date()
            if (!root.showSeconds) {
                // riallinea in caso di deriva o di sospensione/ripresa
                const msToNextMinute = 60000 - (root.now.getSeconds() * 1000 + root.now.getMilliseconds())
                interval = Math.max(1000, msToNextMinute)
            }
        }
        Component.onCompleted: {
            if (!root.showSeconds) {
                const n = new Date()
                interval = Math.max(1000, 60000 - (n.getSeconds() * 1000 + n.getMilliseconds()))
            }
        }
    }

    // ---- Sfondo "vetro" ---------------------------------------------
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0.05, 0.07, 0.10, panelOpacity)
        border.color: Qt.rgba(1, 1, 1, 0.08)
        border.width: 1

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.05) }
                GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.08) }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 8

        // ---- Pulsante di avvio --------------------------------------
        Rectangle {
            id: startBtn
            Layout.preferredHeight: root.barH - 10
            Layout.preferredWidth: startRow.implicitWidth + 20
            radius: 10
            color: startTap.pressed ? Qt.rgba(1, 1, 1, 0.20)
                                    : (startHover.hovered ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(1, 1, 1, 0.05))
            scale: startTap.pressed ? 0.95 : 1.0
            Behavior on color { ColorAnimation { duration: 130 } }
            Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

            RowLayout {
                id: startRow
                anchors.centerIn: parent
                spacing: 7
                Image {
                    source: "qrc:/nicoos/icons/nicoos-shell.svg"
                    sourceSize: Qt.size(44, 44)
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    smooth: true
                }
                Text {
                    text: "NicoOS"
                    color: "#ffffff"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
            }
            HoverHandler { id: startHover }
            TapHandler { id: startTap; onTapped: shell.toggle("launcher") }
        }

        // ---- Puntini dei workspace ----------------------------------
        Rectangle {
            Layout.preferredHeight: 24
            Layout.preferredWidth: wsRow.implicitWidth + 16
            radius: 12
            color: Qt.rgba(1, 1, 1, 0.06)

            // Scorri sopra i puntini per spostarti tra i workspace.
            WheelHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                onWheel: (event) => {
                    if (event.angleDelta.y === 0) return
                    workspaceModel.activateRelative(event.angleDelta.y < 0 ? 1 : -1)
                }
            }

            Row {
                id: wsRow
                anchors.centerIn: parent
                spacing: 0

                Repeater {
                    model: workspaceModel
                    delegate: Item {
                        width: dot.width + 8
                        height: 24

                        Rectangle {
                            id: dot
                            anchors.centerIn: parent
                            width: isActive ? 20 : 8
                            height: 8
                            radius: 4
                            color: isActive ? accentColor : Qt.rgba(1, 1, 1, windowCount > 0 ? 0.6 : 0.25)
                            Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                            Behavior on color { ColorAnimation { duration: 160 } }
                        }
                        TapHandler { onTapped: workspaceModel.activate(wsId) }
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.preferredHeight: root.barH - 18
            color: Qt.rgba(1, 1, 1, 0.12)
        }

        // ---- Elenco finestre (barra delle applicazioni in stile Windows) ----
        ListView {
            id: taskList
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: ListView.Horizontal
            spacing: 4
            clip: true
            interactive: false
            model: taskbarModel

            readonly property real btnWidth: Math.max(
                40, Math.min(180, (width - spacing * Math.max(0, count - 1)) / Math.max(1, count)))

            add: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 180 }
                    NumberAnimation { property: "scale"; from: 0.7; to: 1; duration: 240; easing.type: Easing.OutBack }
                }
            }
            remove: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; to: 0; duration: 140 }
                    NumberAnimation { property: "scale"; to: 0.7; duration: 140 }
                }
            }
            displaced: Transition {
                NumberAnimation { properties: "x"; duration: 220; easing.type: Easing.OutCubic }
            }

            delegate: Item {
                id: cell
                width: taskList.btnWidth
                height: taskList.height
                Behavior on width { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

                Rectangle {
                    id: btn
                    anchors.fill: parent
                    anchors.topMargin: 4
                    anchors.bottomMargin: 4
                    radius: 9
                    opacity: winMinimized ? 0.5 : 1.0
                    scale: btnTap.pressed ? 0.96 : 1.0
                    color: winActive ? Qt.rgba(1, 1, 1, 0.17)
                                     : (btnHover.hovered ? Qt.rgba(1, 1, 1, 0.10) : "transparent")
                    border.width: winActive ? 1 : 0
                    border.color: Qt.rgba(1, 1, 1, 0.20)

                    Behavior on color { ColorAnimation { duration: 140 } }
                    Behavior on opacity { NumberAnimation { duration: 180 } }
                    Behavior on scale { NumberAnimation { duration: 90 } }

                    // barretta d'accento: lunga se attiva, corta al passaggio
                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 1
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: 2
                        radius: 1
                        color: accentColor
                        width: winActive ? parent.width * 0.55 : (btnHover.hovered ? parent.width * 0.22 : 0)
                        Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 9
                        anchors.rightMargin: 6
                        spacing: 8

                        Image {
                            Layout.preferredWidth: root.iconPx
                            Layout.preferredHeight: root.iconPx
                            Layout.alignment: Qt.AlignVCenter
                            source: "image://nicoosicon/" + winIconKey
                            sourceSize: Qt.size(root.iconPx * 2, root.iconPx * 2)
                            smooth: true
                            asynchronous: true
                        }
                        Text {
                            visible: taskList.btnWidth > 100
                            Layout.fillWidth: true
                            text: winTitle.length > 0 ? winTitle : winClass
                            color: "#ffffff"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                        Item {
                            id: closeBtn
                            visible: taskList.btnWidth > 100 && btnHover.hovered
                            Layout.preferredWidth: 18
                            Layout.preferredHeight: 18

                            Rectangle {
                                anchors.fill: parent
                                radius: 9
                                color: closeHover.hovered ? Qt.rgba(1, 0.3, 0.3, 0.5) : "transparent"
                            }
                            IconGlyph {
                                anchors.centerIn: parent
                                source: "qrc:/nicoos/icons/close.svg"
                                iconSize: 9
                            }
                            HoverHandler { id: closeHover }
                            TapHandler { onTapped: taskbarModel.close(winAddress) }
                            readonly property bool hovered: visible && closeHover.hovered
                        }
                    }

                    HoverHandler { id: btnHover }
                    TapHandler {
                        id: btnTap
                        acceptedButtons: Qt.LeftButton
                        // I TapHandler annidati scattano entrambi: salta
                        // l'azione di focus/minimizza quando il clic e'
                        // finito sul pulsante di chiusura.
                        onTapped: if (!closeBtn.hovered) taskbarModel.activate(winAddress)
                    }
                    TapHandler {
                        acceptedButtons: Qt.MiddleButton
                        onTapped: taskbarModel.close(winAddress)
                    }
                }
            }
        }

        // ---- Indicatori di modalita' (solo se attivi) ------------------
        RowLayout {
            spacing: 8
            IconGlyph {
                visible: quick.recording
                source: "qrc:/nicoos/icons/record.svg"
                tint: "#ff5c5c"
                iconSize: 15
                SequentialAnimation on opacity {
                    running: quick.recording
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.3; duration: 650 }
                    NumberAnimation { to: 1.0; duration: 650 }
                }
            }
            IconGlyph { visible: quick.doNotDisturb; source: "qrc:/nicoos/icons/bell-off.svg"; iconSize: 15 }
            IconGlyph { visible: quick.nightLight; source: "qrc:/nicoos/icons/moon.svg"; iconSize: 15 }
            IconGlyph { visible: quick.caffeine; source: "qrc:/nicoos/icons/coffee.svg"; iconSize: 15 }
        }

        // ---- Stato del sistema (clic: centro di controllo) ------------
        Rectangle {
            Layout.preferredHeight: 26
            Layout.preferredWidth: statusRow.implicitWidth + 18
            radius: 9
            color: statusTap.pressed ? Qt.rgba(1, 1, 1, 0.18)
                                     : (statusHover.hovered ? Qt.rgba(1, 1, 1, 0.10) : "transparent")
            Behavior on color { ColorAnimation { duration: 130 } }

            RowLayout {
                id: statusRow
                anchors.centerIn: parent
                spacing: 11

                IconGlyph {
                    source: networkClient.connected ? "qrc:/nicoos/icons/network-wifi.svg"
                                                    : "qrc:/nicoos/icons/network-off.svg"
                    iconSize: root.iconPx - 3
                    // Wi-Fi debole piu' spento; rete via cavo o segnale
                    // forte restano pieni
                    opacity: (networkClient.connected && networkClient.isWifi)
                             ? 0.45 + 0.55 * Math.min(100, networkClient.signalStrength) / 100
                             : 1.0
                    Behavior on opacity { NumberAnimation { duration: 200 } }
                }
                IconGlyph {
                    source: audioClient.muted ? "qrc:/nicoos/icons/audio-muted.svg"
                                              : "qrc:/nicoos/icons/audio-on.svg"
                    iconSize: root.iconPx - 3
                }
                RowLayout {
                    visible: sysMonitor.hasBattery
                    spacing: 4
                    IconGlyph {
                        source: sysMonitor.batteryCharging ? "qrc:/nicoos/icons/battery-charging.svg"
                                                           : "qrc:/nicoos/icons/battery.svg"
                        tint: (sysMonitor.batteryPercent < 20 && !sysMonitor.batteryCharging) ? "#ff6b6b" : "#ffffff"
                        iconSize: root.iconPx - 2
                    }
                    Text {
                        text: sysMonitor.batteryPercent + "%"
                        color: (sysMonitor.batteryPercent < 20 && !sysMonitor.batteryCharging) ? "#ff6b6b" : "#ffffff"
                        font.pixelSize: 12
                    }
                }
                RowLayout {
                    spacing: 4
                    IconGlyph {
                        source: "qrc:/nicoos/icons/cpu.svg"
                        tint: sysMonitor.cpuUsagePercent > 85 ? "#ff6b6b" : "#ffffff"
                        iconSize: root.iconPx - 3
                    }
                    Text {
                        text: sysMonitor.cpuUsagePercent.toFixed(0) + "%"
                        color: sysMonitor.cpuUsagePercent > 85 ? "#ff6b6b" : "#ffffff"
                        font.pixelSize: 12
                    }
                }
                RowLayout {
                    spacing: 4
                    IconGlyph {
                        source: "qrc:/nicoos/icons/memory.svg"
                        tint: sysMonitor.memoryUsagePercent > 90 ? "#ff6b6b" : "#ffffff"
                        iconSize: root.iconPx - 3
                    }
                    Text {
                        text: sysMonitor.memoryUsagePercent.toFixed(0) + "%"
                        color: sysMonitor.memoryUsagePercent > 90 ? "#ff6b6b" : "#ffffff"
                        font.pixelSize: 12
                    }
                }
            }
            HoverHandler { id: statusHover }
            TapHandler { id: statusTap; onTapped: shell.toggle("controlcenter") }
            // Scorri sopra l'area di stato: volume a passi del 5%.
            WheelHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                onWheel: (event) => {
                    if (event.angleDelta.y === 0 || !audioClient.available) return
                    const step = event.angleDelta.y > 0 ? 5 : -5
                    audioClient.setVolume(Math.max(0, Math.min(100, audioClient.volume + step)))
                }
            }
        }

        // ---- Orologio (clic: calendario) ------------------------------
        Rectangle {
            Layout.preferredHeight: 26
            Layout.preferredWidth: clockCol.implicitWidth + 18
            radius: 9
            color: clockTap.pressed ? Qt.rgba(1, 1, 1, 0.18)
                                    : (clockHover.hovered ? Qt.rgba(1, 1, 1, 0.10) : "transparent")
            Behavior on color { ColorAnimation { duration: 130 } }

            RowLayout {
                id: clockCol
                anchors.centerIn: parent
                spacing: 8
                Text {
                    text: Qt.formatDateTime(root.now, "ddd d MMM")
                    color: Qt.rgba(1, 1, 1, 0.7)
                    font.pixelSize: 12
                }
                Text {
                    text: Qt.formatDateTime(root.now, clockFormat)
                    color: "#ffffff"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
            }
            HoverHandler { id: clockHover }
            TapHandler { id: clockTap; onTapped: shell.toggle("calendar") }
        }

        // ---- Pulsante di spegnimento ----------------------------------
        Rectangle {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 26
            radius: 9
            color: powerTap.pressed ? Qt.rgba(1, 0.3, 0.3, 0.45)
                                    : (powerHover.hovered ? Qt.rgba(1, 0.3, 0.3, 0.28) : "transparent")
            Behavior on color { ColorAnimation { duration: 130 } }

            IconGlyph {
                anchors.centerIn: parent
                source: "qrc:/nicoos/icons/power.svg"
                iconSize: root.iconPx - 2
            }
            HoverHandler { id: powerHover }
            TapHandler { id: powerTap; onTapped: shell.toggle("power") }
        }
    }
}
