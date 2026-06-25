import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

Rectangle {
    id: panel
    color: "#16213e"
    radius: 10
    border.color: "#0f3460"
    border.width: 1

    property alias statusText: statusLabel.text

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 4

        // Top row: scan, device selector, connect
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // Scan button
            Button {
                id: scanButton
                text: bluetoothController.scanning ? "扫描中..." : "扫描设备"
                font.pixelSize: 13
                Layout.preferredWidth: 90
                Layout.preferredHeight: 36
                enabled: !bluetoothController.connected
                Material.background: bluetoothController.scanning ? Material.Grey : Material.Blue

                onClicked: {
                    if (bluetoothController.scanning) {
                        bluetoothController.stopDiscovery()
                    } else {
                        bluetoothController.startDiscovery()
                    }
                }
            }

            // Device combo box (with placeholder overlay)
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: deviceCombo.implicitHeight

                ComboBox {
                    id: deviceCombo
                    anchors.fill: parent
                    font.pixelSize: 13
                    model: bluetoothController.discoveredDevices
                    enabled: !bluetoothController.connected
                }

                // Placeholder text when no devices found
                Label {
                    anchors.centerIn: parent
                    text: "点击\"扫描设备\"查找蓝牙设备"
                    font.pixelSize: 13
                    color: "#888"
                    visible: deviceCombo.count === 0 && !bluetoothController.scanning
                }
            }

            // Connect / Disconnect button
            Button {
                id: connectButton
                font.pixelSize: 13
                Layout.preferredWidth: 80
                Layout.preferredHeight: 36
                text: bluetoothController.connected ? "断开" : "连接"
                Material.background: bluetoothController.connected ? Material.Red : Material.Green

                onClicked: {
                    if (bluetoothController.connected) {
                        bluetoothController.disconnectFromDevice()
                    } else if (deviceCombo.currentIndex >= 0) {
                        bluetoothController.connectToDevice(deviceCombo.currentIndex)
                    }
                }
            }
        }

        // Status bar
        Rectangle {
            Layout.fillWidth: true
            height: 18
            radius: 3
            color: "#0a1a3a"

            Label {
                id: statusLabel
                anchors.centerIn: parent
                font.pixelSize: 10
                color: "#aaa"
                text: bluetoothController.statusText || "就绪"
            }
        }
    }
}
