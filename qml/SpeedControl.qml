import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

// Vertical speed slider with +/- buttons
Item {
    id: speedCtrl

    property int speedValue: 128          // 0..255, default mid
    property int speedLevel: Math.round(speedValue / 25.5)  // 0..255 → 0..9
    property int _lastSentLevel: -1       // Throttle: only send when level changes

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        // Speed label
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "速度"
            font.pixelSize: 14
            font.bold: true
            color: "#ccc"
        }

        // Speed level display (0-9)
        Label {
            id: speedLabel
            Layout.alignment: Qt.AlignHCenter
            text: speedCtrl.speedLevel
            font.pixelSize: 22
            font.bold: true
            color: {
                if (speedCtrl.speedLevel <= 2) return "#3498db"
                if (speedCtrl.speedLevel <= 5) return "#f39c12"
                return "#e74c3c"
            }
        }

        // "+" button
        Button {
            id: btnPlus
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 56
            Layout.preferredHeight: 48
            text: "+"
            font.pixelSize: 22
            font.bold: true
            Material.background: Material.Grey

            onClicked: {
                speedCtrl.speedValue = Math.min(255, speedCtrl.speedValue + 26)
                sendSpeedCmd()
            }

            // Repeat while holding
            Timer {
                interval: 150
                running: btnPlus.pressed
                repeat: true
                onTriggered: {
                    speedCtrl.speedValue = Math.min(255, speedCtrl.speedValue + 26)
                    sendSpeedCmd()
                }
            }
        }

        // Vertical Slider
        Slider {
            id: slider
            Layout.alignment: Qt.AlignHCenter
            Layout.fillHeight: true
            Layout.preferredWidth: 50

            orientation: Qt.Vertical
            from: 0
            to: 255
            stepSize: 1
            value: 128

            onValueChanged: {
                speedCtrl.speedValue = Math.round(value)
                sendSpeedCmd()
            }

            background: Rectangle {
                x: parent.width / 2 - width / 2
                y: 0
                implicitWidth: 8
                implicitHeight: parent.height
                radius: 4
                color: "#2a2a4a"
                border.color: "#444"

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    radius: parent.radius
                    height: parent.height * (slider.value / slider.to)
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#e74c3c" }
                        GradientStop { position: 0.5; color: "#f39c12" }
                        GradientStop { position: 1.0; color: "#3498db" }
                    }
                }
            }

            handle: Rectangle {
                x: parent.width / 2 - width / 2
                y: slider.visualPosition * (slider.availableHeight - height)
                width: 36
                height: 24
                radius: 6
                color: slider.pressed ? "#ff9800" : "#e0e0e0"
                border.color: "#888"
                border.width: 1
                Behavior on color { ColorAnimation { duration: 100 } }
            }
        }

        // "-" button
        Button {
            id: btnMinus
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 56
            Layout.preferredHeight: 48
            text: "−"
            font.pixelSize: 24
            font.bold: true
            Material.background: Material.Grey

            onClicked: {
                speedCtrl.speedValue = Math.max(0, speedCtrl.speedValue - 26)
                sendSpeedCmd()
            }

            Timer {
                interval: 150
                running: btnMinus.pressed
                repeat: true
                onTriggered: {
                    speedCtrl.speedValue = Math.max(0, speedCtrl.speedValue - 26)
                    sendSpeedCmd()
                }
            }
        }
    }

    // Send speed command: "ON0" ~ "ON9" (throttled — only when level changes)
    function sendSpeedCmd() {
        var level = Math.min(9, Math.max(0, Math.round(speedCtrl.speedValue / 25.5)))
        if (level !== speedCtrl._lastSentLevel) {
            speedCtrl._lastSentLevel = level
            bluetoothController.sendCommandString("ON" + level)
        }
    }
}
