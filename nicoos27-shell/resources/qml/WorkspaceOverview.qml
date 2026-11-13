import QtQuick
import QtQuick.Layouts
import QtQuick.Effects

Rectangle {
    id: root
    anchors.fill: parent
    radius: 22
    color: Qt.rgba(0.02, 0.02, 0.02, 0.93)
    border.color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.35)
    border.width: 1

    property string draggedWindowAddress: ""
    property string draggedWindowClass: ""
    property bool isDragging: draggedWindowAddress.length > 0

    focus: true
    Keys.onEscapePressed: surface.hideSurface()

    Connections {
        target: surface
        function onVisibleChanged() {
            if (surface.visible) {
                searchInput.text = ""
                searchInput.forceActiveFocus()
            }
        }
    }

    // Sfumatura luminosa di sfondo, puramente decorativa — rafforza il
    // tono fantascientifico "Extreme" senza rubare attenzione ai contenuti.
    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.05) }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        spacing: 16

        // --- Intestazione: titolo + ricerca unificata ---
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            ColumnLayout {
                spacing: 2
                Text {
                    text: "Multitasking"
                    color: "#e3272b"
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                }
                Text {
                    text: windowModel.rowCount() + " finestre attive · " + workspaceModel.rowCount() + " aree di lavoro"
                    color: Qt.rgba(0.89, 0.15, 0.17, 0.45)
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                Layout.preferredWidth: 340
                Layout.preferredHeight: 42
                radius: 12
                color: Qt.rgba(0.89, 0.15, 0.17, 0.05)
                border.color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, searchInput.activeFocus ? 0.55 : 0.25)
                border.width: searchInput.activeFocus ? 1.4 : 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    Image {
                        source: "qrc:/nicoos/icons/search.svg"
                        sourceSize: Qt.size(28, 28)
                        Layout.preferredWidth: 14
                        Layout.preferredHeight: 14
                        opacity: 0.55
                    }

                    TextInput {
                        id: searchInput
                        Layout.fillWidth: true
                        color: "#e3272b"
                        font.pixelSize: 13
                        onTextChanged: windowModel.filterText = text

                        Text {
                            visible: searchInput.text.length === 0
                            text: "Cerca finestre per titolo o app…"
                            color: Qt.rgba(0.89, 0.15, 0.17, 0.35)
                            font.pixelSize: 13
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }

        // --- Griglia principale: una scheda per workspace, ognuna con le
        //     proprie miniature delle finestre ---
        GridView {
            id: wsGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            readonly property int columns: Math.max(1, Math.floor(width / 420))
            cellWidth: width / columns
            // Con le aree configurate (default 10) le schede possono essere
            // molte: lascia che si restringano invece di sfondare la vista.
            cellHeight: Math.max(170, height / Math.max(1, Math.ceil(workspaceModel.rowCount() / columns)))

            model: workspaceModel

            delegate: Item {
                width: wsGrid.cellWidth
                height: wsGrid.cellHeight

                Rectangle {
                    id: card
                    anchors.fill: parent
                    anchors.margins: 10
                    radius: 18
                    color: isActive
                           ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.10)
                           : Qt.rgba(0.89, 0.15, 0.17, 0.035)
                    border.color: dropArea.containsDrag
                           ? accentColor
                           : (isActive ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.5)
                                       : Qt.rgba(0.89, 0.15, 0.17, 0.1))
                    border.width: dropArea.containsDrag ? 2.2 : (isActive ? 1.5 : 1)

                    Behavior on border.color { ColorAnimation { duration: 120 } }
                    Behavior on color { ColorAnimation { duration: 150 } }

                    scale: dropArea.containsDrag ? 1.015 : 1.0
                    Behavior on scale { NumberAnimation { duration: animSpeedMs; easing.type: Easing.OutCubic } }

                    DropArea {
                        id: dropArea
                        anchors.fill: parent
                        onDropped: function(drop) {
                            windowModel.moveWindowToWorkspace(root.draggedWindowAddress, wsId)
                            root.draggedWindowAddress = ""
                            root.draggedWindowClass = ""
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            Rectangle {
                                width: 9; height: 9; radius: 4.5
                                color: isActive ? accentColor : Qt.rgba(0.89, 0.15, 0.17, 0.3)
                            }
                            Text {
                                text: wsName
                                color: "#e3272b"
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                Layout.fillWidth: true
                            }
                            Text {
                                text: windowCount
                                color: Qt.rgba(0.89, 0.15, 0.17, 0.4)
                                font.pixelSize: 11
                            }
                        }

                        // Griglia di miniature aggiornata per le finestre di
                        // questo workspace. Estrae la propria piccola fetta
                        // con windowModel.windowsForWorkspace(wsId) invece di
                        // legarsi direttamente al windowModel condiviso,
                        // cosi' ogni scheda istanzia delegate solo per le
                        // proprie finestre — non un delegate (nascosto) per
                        // ogni scheda per ogni finestra della sessione.
                        // Ricalcolata ogni volta che windowModel fa reset
                        // (refresh()).
                        GridView {
                            id: winGrid
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            cellWidth: width / Math.max(1, Math.min(3, Math.ceil(Math.sqrt(Math.max(1, model.length)))))
                            cellHeight: cellWidth * 0.62

                            model: windowModel.windowsForWorkspace(wsId)

                            function recompute() {
                                model = windowModel.windowsForWorkspace(wsId)
                            }

                            Component.onCompleted: recompute()
                            Connections {
                                target: windowModel
                                function onModelReset() { winGrid.recompute() }
                            }

                            delegate: Loader {
                                width: winGrid.cellWidth
                                height: winGrid.cellHeight
                                sourceComponent: windowThumbComponent
                                property string winAddress: modelData.winAddress
                                property string winClass: modelData.winClass
                                property string winTitle: modelData.winTitle
                                property int winWorkspaceId: modelData.winWorkspaceId
                            }

                            Text {
                                anchors.centerIn: parent
                                visible: windowCount === 0
                                text: "Vuoto"
                                color: Qt.rgba(0.89, 0.15, 0.17, 0.25)
                                font.pixelSize: 12
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        z: -1
                        onClicked: workspaceModel.activate(wsId)
                    }
                }
            }
        }
    }

    // --- Delegate riutilizzabile per la miniatura di finestra
    //     (sorgente del trascinamento) ---
    Component {
        id: windowThumbComponent

        Rectangle {
            id: thumb
            radius: 10
            color: Qt.rgba(0.02, 0.02, 0.02, 0.9)
            border.color: Qt.rgba(0.89, 0.15, 0.17, 0.12)
            border.width: 1
            clip: true

            property point dragStartPos: Qt.point(0, 0)

            Image {
                id: previewImg
                anchors.fill: parent
                anchors.margins: 1
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                visible: status === Image.Ready
            }

            // Popolata dal segnale previewReady di requestPreview();
            // il contesto del Loader ci passa winAddress/winClass/winTitle
            // direttamente dalla riga di WindowListModel.
            Connections {
                target: previewCapture
                function onPreviewReady(address, image) {
                    if (address === winAddress) {
                        previewImg.source = ""  // forza il ricaricamento: la stessa chiave puo' aggiornarsi
                        previewImg.source = "image://windowpreview/" + winAddress
                    }
                }
            }

            Component.onCompleted: previewCapture.requestPreview(winAddress, winClass)

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 6
                spacing: 1

                Text {
                    text: winTitle.length > 0 ? winTitle : winClass
                    color: "#e3272b"
                    font.pixelSize: 11
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    style: Text.Outline
                    styleColor: Qt.rgba(0, 0, 0, 0.6)
                }
            }

            Rectangle {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 4
                width: 18; height: 18; radius: 9
                color: Qt.rgba(0, 0, 0, 0.5)
                visible: closeHover.containsMouse || thumb.state === "dragging"

                Text {
                    anchors.centerIn: parent
                    text: "\u2715"
                    color: "#ffb3b3"
                    font.pixelSize: 10
                }
                MouseArea {
                    id: closeHover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: windowModel.closeWindow(winAddress)
                }
            }

            states: State {
                name: "dragging"
                when: dragArea.drag.active
                ParentChange { target: thumb; parent: root }
                PropertyChanges { thumb.scale: 1.08; thumb.opacity: 0.92; thumb.z: 999 }
            }

            Behavior on scale { NumberAnimation { duration: 120 } }

            MouseArea {
                id: dragArea
                anchors.fill: parent
                hoverEnabled: true
                drag.target: thumb
                drag.axis: Drag.XAndYAxis

                onPressed: {
                    root.draggedWindowAddress = winAddress
                    root.draggedWindowClass = winClass
                }
                onReleased: {
                    thumb.x = 0; thumb.y = 0
                    root.draggedWindowAddress = ""
                    root.draggedWindowClass = ""
                }
                onDoubleClicked: {
                    windowModel.focusWindow(winAddress)
                    workspaceModel.activate(winWorkspaceId)
                }
            }

            // Leggero sollevamento al passaggio del mouse, per far capire
            // che le miniature sono trascinabili prima che l'utente lo
            // scopra da solo.
            scale: dragArea.containsMouse && !dragArea.drag.active ? 1.03 : 1.0
        }
    }
}
