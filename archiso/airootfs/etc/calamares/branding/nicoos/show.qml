/* === NicoOS 27 Extreme — presentazione dell'installer ===
 *
 * Sostituisce la presentazione predefinita di Calamares. Quella lasciava
 * trasparente lo sfondo delle slide, quindi alcune diapositive comparivano
 * con il colore estraneo del contenitore.
 *
 * Qui lo sfondo è un rettangolo nero opaco e colori di testo e titoli sono
 * impostati esplicitamente: titoli e testo nel rosso di NicoOS (#e3272b e la
 * sua variante più chiara #ff6b6b), gli stessi due colori primari del logo.
 */

import QtQuick 2.0
import calamares.slideshow 1.0

Presentation {
    id: presentation

    // Testo rosso su fondo nero, titoli nel rosso del logo.
    textColor: "#ff6b6b"
    titleColor: "#e3272b"
    fontFamily: "DejaVu Sans"

    // Sfondo opaco: nessuna slide può più mostrare il colore del contenitore.
    Rectangle {
        anchors.fill: parent
        z: -2
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0;  color: "#000000" }
            GradientStop { position: 0.55; color: "#0a0a0a" }
            GradientStop { position: 1.0;  color: "#141414" }
        }
    }

    // Barretta d'accento in alto, nel rosso del logo.
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 3
        z: -1
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#e3272b" }
            GradientStop { position: 1.0; color: "#b41e22" }
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
        opacity: 1.0
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
