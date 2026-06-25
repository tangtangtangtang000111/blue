import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Cross-shaped direction pad (十字方向键)
// Protocol: press = send "ONx", release = send "ONF" (only if no other button held)
Item {
    id: dpad

    property real btnSize: Math.min(width, height) * 0.3

    // Reset all states when Bluetooth disconnects
    Connections {
        target: bluetoothController
        function onConnectedChanged() {
            if (!bluetoothController.connected) {
                forwardActive = false
                backwardActive = false
                leftActive = false
                rightActive = false
            }
        }
    }

    // Protocol strings
    readonly property string cmdForward:  "ONA"
    readonly property string cmdBackward: "ONB"
    readonly property string cmdLeft:     "ONC"
    readonly property string cmdRight:    "OND"
    readonly property string cmdStop:     "ONE"
    readonly property string cmdOff:      "ONF"

    // Track which directions are active
    property bool forwardActive: false
    property bool backwardActive: false
    property bool leftActive: false
    property bool rightActive: false
    property bool anyDirectionActive: forwardActive || backwardActive || leftActive || rightActive

    // Send the currently active direction command
    function sendActiveDirection() {
        if (forwardActive) {
            bluetoothController.sendCommandString(cmdForward)
        } else if (backwardActive) {
            bluetoothController.sendCommandString(cmdBackward)
        } else if (leftActive) {
            bluetoothController.sendCommandString(cmdLeft)
        } else if (rightActive) {
            bluetoothController.sendCommandString(cmdRight)
        }
    }

    // Called when any direction button is released
    function onDirectionRelease() {
        if (anyDirectionActive) {
            sendActiveDirection()
        } else {
            bluetoothController.sendCommandString(cmdOff)
        }
    }

    // ---- UP button ----
    DPadButton {
        id: btnUp
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        width: btnSize
        height: btnSize
        label: "↑"
        btnColor: "#1a3a5c"

        onPressed: {
            forwardActive = true
            bluetoothController.sendCommandString(cmdForward)
        }
        onReleased: {
            forwardActive = false
            onDirectionRelease()
        }
    }

    // ---- DOWN button ----
    DPadButton {
        id: btnDown
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        width: btnSize
        height: btnSize
        label: "↓"
        btnColor: "#1a3a5c"

        onPressed: {
            backwardActive = true
            bluetoothController.sendCommandString(cmdBackward)
        }
        onReleased: {
            backwardActive = false
            onDirectionRelease()
        }
    }

    // ---- LEFT button ----
    DPadButton {
        id: btnLeft
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        width: btnSize
        height: btnSize
        label: "←"
        btnColor: "#1a3a5c"

        onPressed: {
            leftActive = true
            bluetoothController.sendCommandString(cmdLeft)
        }
        onReleased: {
            leftActive = false
            onDirectionRelease()
        }
    }

    // ---- RIGHT button ----
    DPadButton {
        id: btnRight
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        width: btnSize
        height: btnSize
        label: "→"
        btnColor: "#1a3a5c"

        onPressed: {
            rightActive = true
            bluetoothController.sendCommandString(cmdRight)
        }
        onReleased: {
            rightActive = false
            onDirectionRelease()
        }
    }

    // ---- STOP button (center) ----
    DPadButton {
        id: btnStop
        anchors.centerIn: parent
        width: btnSize * 0.9
        height: btnSize * 0.9
        label: "停"
        btnColor: "#c0392b"
        labelColor: "#fff"

        onPressed: {
            forwardActive = false
            backwardActive = false
            leftActive = false
            rightActive = false
            bluetoothController.sendCommandString(cmdStop)
        }
    }
}
