import QtQuick 2.15
import QtQuick.Controls 2.15

// A single direction pad button
Rectangle {
    id: btn
    property string label: ""
    property color btnColor: "#1a3a5c"
    property color labelColor: "#e0e0e0"

    // Forward pressed/released from internal MouseArea
    signal pressed()
    signal released()

    radius: 10
    color: mouseArea.pressed ? Qt.lighter(btnColor, 1.4) : btnColor
    border.color: Qt.lighter(btnColor, 1.2)
    border.width: 2

    Behavior on color {
        ColorAnimation { duration: 80 }
    }

    scale: mouseArea.pressed ? 0.92 : 1.0
    Behavior on scale {
        NumberAnimation { duration: 80; easing.type: Easing.OutBack }
    }

    Text {
        anchors.centerIn: parent
        text: label
        font.pixelSize: Math.min(btn.width, btn.height) * 0.4
        font.bold: true
        color: labelColor
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent

        onPressed: btn.pressed()
        onReleased: btn.released()
    }
}
