import QtQuick 2.12
import QtQuick.Window 2.12

Window {
    id: root
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowMaximizeButtonHint
    width: 800
    height: 600
    color: "transparent"

    property real backgroundRadius: 4
    property bool backgroundHasBorder: true
    property color backgroundColor: "#2E2F30"
    property color backgroundBorderColor: "#555656"

    Rectangle {
        id: backgroundView
        anchors.fill: parent
        color: root.backgroundColor
        radius: root.backgroundRadius
        clip: true
        antialiasing: true

        border {
            color: root.backgroundHasBorder ? root.backgroundBorderColor : "transparent"
            width: root.backgroundHasBorder ? 1 : 0
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 60
        color: "transparent"

        DragHandler {
            grabPermissions: TapHandler.CanTakeOverFromAnything
            onActiveChanged: if (active) { root.startSystemMove(); }
        }
    }
}
