import QtQuick
import QtQuick.Effects

// Icona della shell. Gli SVG sono disegnati in bianco pieno, quindi con la
// tinta predefinita vengono mostrati cosi' come sono (nessun effetto,
// nessun costo). Per qualsiasi altra tinta (es. avvisi rossi) l'immagine
// bianca viene ricolorata con MultiEffect.
Item {
    id: root
    property alias source: img.source
    property color tint: "#ffffff"
    property int iconSize: 16
    readonly property bool plain: Qt.colorEqual(tint, "#ffffff")
    // MultiEffect richiede shader; il renderer software non puo' disegnarlo,
    // quindi l'icona tinta sparirebbe. In quel caso mostra l'icona semplice.
    readonly property bool software: GraphicsInfo.api === GraphicsInfo.Software

    implicitWidth: iconSize
    implicitHeight: iconSize
    width: iconSize
    height: iconSize

    Image {
        id: img
        anchors.fill: parent
        sourceSize: Qt.size(root.iconSize * 2, root.iconSize * 2)
        smooth: true
        asynchronous: true
        visible: root.plain || root.software
    }

    MultiEffect {
        anchors.fill: parent
        source: img
        visible: !root.plain && !root.software
        colorization: 1.0
        colorizationColor: root.tint
    }
}
