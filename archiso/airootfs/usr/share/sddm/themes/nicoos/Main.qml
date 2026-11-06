
import QtQuick
import QtQuick.Window 2.15

Rectangle {
    id: root

    width: Screen.width
    height: Screen.height
    color: "#05070d"

    property string accent: "#00d9ff"
    property string accent2: "#7a5cff"
    property string errorText: ""
    property bool loggingIn: false
    property string userName: "nicoos"

    // Sfondo
    Image {
        anchors.fill: parent
        source: config.background ? config.background : "background.png"
        fillMode: Image.PreserveAspectCrop
        smooth: true
        asynchronous: true
    }

    Rectangle {
        anchors.fill: parent
        color: "#05070d"
        opacity: 0.45
    }

    // Benvenuto
    Column {
        anchors.left: parent.left
        anchors.leftMargin: 80
        anchors.verticalCenter: parent.verticalCenter

        Text {
            text: "Benvenuto."
            color: "#e6f5ff"
            font.family: "JetBrainsMono Nerd Font"
            font.pixelSize: 42
            font.bold: true
        }
    }

    // Scheda di accesso
    Rectangle {
        id: card
        width: 400
        height: 336
        anchors.right: parent.right
        anchors.rightMargin: 140
        anchors.verticalCenter: parent.verticalCenter
        radius: 14
        color: "#0b1220"
        border.width: 1
        border.color: "#1b2b3d"
        opacity: 0.97

        Column {
            anchors.fill: parent
            anchors.margins: 32
            spacing: 14

            Text {
                text: "Accedi"
                color: "#e6f5ff"
                font.family: "JetBrainsMono Nerd Font"
                font.pixelSize: 24
                font.bold: true
            }

            Item { width: 1; height: 8 }

            Text {
                text: "NOME UTENTE"
                color: "#5a6b80"
                font.family: "JetBrainsMono Nerd Font"
                font.pixelSize: 10
                font.letterSpacing: 2
            }

            Rectangle {
                width: parent.width
                height: 42
                radius: 8
                color: "#070c14"
                border.width: 1
                border.color: userField.activeFocus
                    ? root.accent : "#1b2b3d"

                TextInput {
                    id: userField
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    verticalAlignment: TextInput.AlignVCenter
                    color: "#e6f5ff"
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 14
                    selectByMouse: true
                    text: root.userName

                    onAccepted: passwordField.forceActiveFocus()
                }
            }

            Text {
                text: "PASSWORD"
                color: "#5a6b80"
                font.family: "JetBrainsMono Nerd Font"
                font.pixelSize: 10
                font.letterSpacing: 2
            }

            Rectangle {
                width: parent.width
                height: 42
                radius: 8
                color: "#070c14"
                border.width: 1
                border.color: passwordField.activeFocus
                    ? root.accent : "#1b2b3d"

                TextInput {
                    id: passwordField
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    verticalAlignment: TextInput.AlignVCenter
                    color: "#e6f5ff"
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 14
                    echoMode: TextInput.Password
                    passwordCharacter: "•"
                    selectByMouse: true

                    onAccepted: root.tryLogin()
                    onTextChanged: root.errorText = ""
                }
            }

            Text {
                text: root.errorText
                color: "#ff5c7a"
                visible: root.errorText !== ""
                font.family: "JetBrainsMono Nerd Font"
                font.pixelSize: 11
                elide: Text.ElideRight
                width: parent.width
            }

            Rectangle {
                width: parent.width
                height: 42
                radius: 8

                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop {
                        position: 0.0
                        color: root.accent
                    }
                    GradientStop {
                        position: 1.0
                        color: root.accent2
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: root.loggingIn
                        ? "ACCESSO IN CORSO…" : "ACCEDI"
                    color: "#05070d"
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 13
                    font.bold: true
                    font.letterSpacing: 2
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.tryLogin()
                }
            }
        }
    }

    // Orologio
    Text {
        id: clock
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 48
        color: "#e6f5ff"
        font.family: "JetBrainsMono Nerd Font"
        font.pixelSize: 40
        font.bold: true

        function tick() {
            var d = new Date()
            var hh = ("0" + d.getHours()).slice(-2)
            var mm = ("0" + d.getMinutes()).slice(-2)
            text = hh + ":" + mm
        }

        Component.onCompleted: tick()
    }

    Timer {
        interval: 10000
        running: true
        repeat: true
        onTriggered: clock.tick()
    }

    // Comportamento dell'accesso
    function tryLogin() {
        if (root.loggingIn)
            return

        if (userField.text.length === 0) {
            root.errorText = "Inserisci un nome utente"
            return
        }

        root.loggingIn = true
        root.errorText = ""

        var idx = 0

        try {
            if (sessionModel.lastIndex !== undefined)
                idx = sessionModel.lastIndex
        } catch (e) {
            idx = 0
        }

        sddm.login(userField.text, passwordField.text, idx)
    }

    Connections {
        target: sddm

        function onLoginFailed() {
            root.loggingIn = false
            root.errorText =
                "Autenticazione non riuscita — password errata?"
            passwordField.text = ""
            passwordField.forceActiveFocus()
        }

        function onLoginSucceeded() {
            root.loggingIn = true
        }
    }

    Component.onCompleted: {
        try {
            if (userModel.lastUser &&
                userModel.lastUser !== "") {
                root.userName = userModel.lastUser
            }
        } catch (e) {
            // Mantieni il nome utente predefinito
        }

        if (passwordField)
            passwordField.forceActiveFocus()
    }
}
