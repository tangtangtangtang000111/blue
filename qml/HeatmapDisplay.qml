import QtQuick 2.15
import QtQuick.Controls 2.15

// Display area for the interpolated heatmap
Rectangle {
    id: display
    color: "#0a0a1a"
    radius: 10
    border.color: "#1a1a4a"
    border.width: 2

    // Title
    Label {
        id: titleLabel
        anchors.top: parent.top
        anchors.topMargin: 4
        anchors.horizontalCenter: parent.horizontalCenter
        text: "热力图传感器"
        font.pixelSize: 13
        font.bold: true
        color: "#888"
    }

    // Heatmap image from image provider
    // Source URL includes version — QML auto-refreshes when version changes, no Timer needed
    Image {
        id: heatmapImage
        anchors.fill: parent
        anchors.margins: 6
        anchors.topMargin: 24

        source: "image://heatmap/current?v=" + bluetoothController.heatmapVersion
        cache: false
        fillMode: Image.PreserveAspectFit
        smooth: true
    }

    // ---- "No Data" overlay ----
    Rectangle {
        anchors.fill: parent
        anchors.margins: 6
        anchors.topMargin: 24
        color: "#0a0a1a"
        visible: !bluetoothController.hasHeatmapData
        radius: 6

        Column {
            anchors.centerIn: parent
            spacing: 8

            // Icon: empty grid
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "📡"
                font.pixelSize: 40
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: {
                    if (!bluetoothController.connected)
                        return "未连接蓝牙"
                    else
                        return "等待热力图数据..."
                }
                font.pixelSize: 15
                color: "#aaa"
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "32×24 矩阵帧格式: AA 55 + 768字节 + 55 AA"
                font.pixelSize: 10
                color: "#666"
                visible: bluetoothController.connected
            }
        }
    }

    // Temperature scale legend (only when data is available)
    Row {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 2
        visible: bluetoothController.hasHeatmapData

        Rectangle {
            width: 160
            height: 16
            radius: 3
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#000080" }
                GradientStop { position: 0.25; color: "#00ffff" }
                GradientStop { position: 0.5; color: "#00ff00" }
                GradientStop { position: 0.75; color: "#ffff00" }
                GradientStop { position: 1.0; color: "#ff0000" }
            }
        }

        Label {
            text: "冷"
            font.pixelSize: 10
            color: "#00aaff"
            anchors.verticalCenter: parent.verticalCenter
        }

        Label {
            text: "热"
            font.pixelSize: 10
            color: "#ff4444"
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
