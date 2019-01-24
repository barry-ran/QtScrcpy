import QtQuick 2.5
import QtGraphicalEffects 1.0
Image {
    Image {
        id: wheel
        anchors.centerIn: parent
        source: "images/pinwheel.png"

        RotationAnimation {
            id:rotationAnimation
            target: wheel
            to:360000
            direction: RotationAnimation.Clockwise
            duration: 800000
        }

        onStatusChanged: if (wheel.status == Image.Ready) rotationAnimation.start()
    }
}