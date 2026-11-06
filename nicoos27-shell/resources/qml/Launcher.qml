import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    anchors.fill: parent
    color: Qt.rgba(0.04, 0.05, 0.07, 0.88)
    radius: 18
    border.color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.35)
    border.width: 1

    property int selectedIndex: 0

    focus: true
    Keys.onEscapePressed: surface.hideSurface()

    Connections {
        target: surface
        function onVisibleChanged() {
            if (surface.visible) {
                searchInput.text = ""
                root.selectedIndex = 0
                searchInput.forceActiveFocus()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.03) }
            GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.08) }
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        // Campo di ricerca
        Rectangle {
            width: parent.width
            height: 46
            radius: 12
            color: Qt.rgba(1, 1, 1, 0.05)
            border.color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.4)
            border.width: searchInput.activeFocus ? 1.4 : 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 10

                Text {
                    text: "\u2315"
                    color: accentColor
                    font.pixelSize: 16
                }

                TextInput {
                    id: searchInput
                    Layout.fillWidth: true
                    color: "#eaf9ff"
                    font.pixelSize: 16
                    focus: true
                    onTextChanged: {
                        launcherModel.query = text
                        root.selectedIndex = 0
                    }
                    Keys.onDownPressed: root.selectedIndex = Math.min(root.selectedIndex + 1, launcherModel.rowCount() - 1)
                    Keys.onUpPressed: root.selectedIndex = Math.max(root.selectedIndex - 1, 0)
                    Keys.onReturnPressed: launcherModel.launch(root.selectedIndex)
                    Keys.onEnterPressed: launcherModel.launch(root.selectedIndex)

                    Text {
                        visible: searchInput.text.length === 0
                        text: "Cerca applicazioni…"
                        color: Qt.rgba(1, 1, 1, 0.35)
                        font.pixelSize: 16
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        // Risultati
        ListView {
            width: parent.width
            height: parent.height - 60
            model: launcherModel
            clip: true
            currentIndex: root.selectedIndex
            spacing: 4

            // Ogni voce usa la stessa griglia: colore di sfondo, icona,
            // testo. Le misure sono fisse e uguali per tutte le righe, così
            // la voce selezionata ha esattamente la stessa geometria delle
            // altre e i nomi partono tutti dalla stessa X.
            delegate: Rectangle {
                id: entry
                width: ListView.view.width
                height: entryHeight
                radius: 10
                color: index === root.selectedIndex
                       ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.16)
                       : "transparent"
                border.color: index === root.selectedIndex
                       ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.4)
                       : "transparent"
                border.width: 1

                // Un unico punto da cui leggere le misure della griglia: la
                // larghezza della colonna icona e i margini interni.
                readonly property int entryHeight: 52
                readonly property int padH: 12
                readonly property int iconSize: 32
                readonly property int gap: 12

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: entry.padH
                    anchors.rightMargin: entry.padH
                    spacing: entry.gap

                    // Colonna dell'icona a larghezza fissa. Il contenitore è un
                    // Item vuoto di 32x32: un Image nel layout verrebbe
                    // allargato dal proprio minimo implicito (la dimensione
                    // reale del file caricato, o 0 finché il caricamento
                    // asincrono non è finito), quindi la colonna cambierebbe
                    // larghezza riga per riga e i nomi partirebbero da X
                    // diverse.
                    Item {
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: entry.iconSize
                        Layout.minimumWidth: entry.iconSize
                        Layout.maximumWidth: entry.iconSize
                        Layout.preferredHeight: entry.iconSize
                        Layout.minimumHeight: entry.iconSize
                        Layout.maximumHeight: entry.iconSize

                        Image {
                            anchors.fill: parent
                            source: "image://nicoosicon/" + iconName
                            sourceSize: Qt.size(entry.iconSize * 2, entry.iconSize * 2)
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            asynchronous: true
                        }
                    }

                    // Nome e descrizione sono impilati e occupano tutta la
                    // larghezza che resta: l'elide lavora quindi sulla
                    // larghezza vera della riga, non su una misura parziale.
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 1

                        Text {
                            Layout.fillWidth: true
                            text: name
                            color: "#eaf9ff"
                            font.pixelSize: 14
                            elide: Text.ElideRight
                            maximumLineCount: 1
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: comment.length > 0
                            text: comment
                            color: Qt.rgba(1, 1, 1, 0.45)
                            font.pixelSize: 11
                            elide: Text.ElideRight
                            maximumLineCount: 1
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: launcherModel.launch(index)
                }
            }
        }
    }
}
