import QtQuick 2.12
import QtQuick.Window 2.12

Window {
    id: window
    visible: true
    flags: Qt.Window |Qt.FramelessWindowHint
    width: 800
    height: 600
    color: "transparent"

    // bg
    Image {
        id: background
        //anchors { top: parent.top; bottom: parent.bottom }
        anchors.fill: parent
        source: "qrc:/image/mainwindow/bg.png"
        fillMode: Image.PreserveAspectCrop
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 30
        color: "red"

        DragHandler {
            grabPermissions: TapHandler.CanTakeOverFromAnything
            onActiveChanged: if (active) { window.startSystemMove(); }
        }
    }

    DragHandler {
        id: resizeHandler
        grabPermissions: TapHandler.TakeOverForbidden
        target: null
        onActiveChanged:
            if (active) {
                const p = resizeHandler.centroid.position;
                let e = 0;
                if (p.x / width < 0.10) { e |= Qt.LeftEdge }
                if (p.x / width > 0.90) { e |= Qt.RightEdge }
                if (p.y / height > 0.90) { e |= Qt.BottomEdge }
                console.log("RESIZING", e);
                window.startSystemResize(e);
            }
    }
}
