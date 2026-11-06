/* === NicoOS 27 Extreme — presentazione dell'installer ===
 *
 * Sostituisce la presentazione predefinita di Calamares. Quella lasciava
 * trasparente lo sfondo delle slide: il contenitore QtQuick dietro di esse è
 * bianco, quindi alcune diapositive comparivano completamente bianche.
 *
 * Qui lo sfondo è un rettangolo scuro opaco, e colori di testo e titoli sono
 * impostati esplicitamente (i valori predefiniti sono nero su bianco): la
 * presentazione resta coerente con il dark mode del resto di NicoOS.
 */

import QtQuick 2.0
import calamares.slideshow 1.0

Presentation {
    id: presentation

    // Testi chiari su fondo scuro.
    textColor: "#dbe9f6"
    titleColor: "#00d9ff"
    fontFamily: "DejaVu Sans"

    // Sfondo opaco: nessuna slide può più mostrare il bianco del contenitore.
    Rectangle {
        anchors.fill: parent
        z: -2
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0;  color: "#0a1220" }
            GradientStop { position: 0.55; color: "#05070d" }
            GradientStop { position: 1.0;  color: "#04060b" }
        }
    }

    // Barretta d'accento in alto, la stessa dei pannelli della shell.
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 3
        z: -1
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#00d9ff" }
            GradientStop { position: 1.0; color: "#7a5cff" }
        }
    }

    Image {
        source: "logo.png"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 16
        width: 92
        height: 92
        fillMode: Image.PreserveAspectFit
        opacity: 0.85
        z: -1
    }

    Timer {
        interval: 6000
        running: presentation.activatedInCalamares
        repeat: true
        onTriggered: presentation.goToNextSlide()
    }

    Slide {
        title: "Benvenuto in NicoOS 27 Extreme"
        content: [
            "Un desktop Arch Linux già pronto: Hyprland, SDDM e la shell dei sogni",
            "Preparati per la prima vera distro della NicoOfficial.",
            "usa Arch Linux senza andare attraverso l'inferno della configurazione."
        ]
    }

    Slide {
        title: "Una shell, tutto a portata di mano"
        content: [
            "Pannello con aree di lavoro e finestre, come una barra delle applicazioni.",
            "SUPER + R apre il launcher, SUPER + N il centro di controllo.",
            "Notifiche, calendario, appunti, sfondi e screenshot inclusi."
        ]
    }

    Slide {
        title: "Prima di iniziare"
        content: [
            "Servono un nome utente, la password dell'account e il fuso orario.",
            "Controlla la tastiera: NicoOS è configurata in italiano.",
            "La partizione scelta viene formattata: i dati presenti andranno persi."
        ]
    }

    Slide {
        title: "Installazione in corso"
        content: [
            "La copia del sistema richiede qualche minuto.",
            "Alla fine l'installer verifica initramfs, bootloader e desktop.",
            "Non spegnere il computer finché non compare il messaggio finale."
        ]
    }

    // Con la presentazione caricata in "V2" vengono chiamate anche queste.
    function onActivate() {
        presentation.currentSlide = 0;
    }

    function onLeave() {
    }
}
