import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

ApplicationWindow {
    id: root
    visible: true
    width: 480
    height: 800
    title: "蓝牙小车遥控器"

    Material.theme: Material.Dark
    Material.accent: Material.Orange

    // Main layout
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        // ---- Bluetooth Panel ----
        BluetoothPanel {
            id: bluetoothPanel
            Layout.fillWidth: true
            Layout.preferredHeight: 72
        }

        // ---- Heatmap Display ----
        HeatmapDisplay {
            id: heatmapDisplay
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // ---- Controls Area ----
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1a1a2e"
            radius: 12

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 16

                // Direction Pad (十字键)
                DirectionPad {
                    id: directionPad
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: parent.width * 0.65
                }

                // Separator line
                Rectangle {
                    Layout.preferredWidth: 2
                    Layout.fillHeight: true
                    color: "#333355"
                }

                // Speed Control
                SpeedControl {
                    id: speedControl
                    Layout.preferredWidth: 80
                    Layout.fillHeight: true
                }
            }
        }
    }
}
