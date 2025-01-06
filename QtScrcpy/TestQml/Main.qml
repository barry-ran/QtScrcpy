import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtScrcpy

ApplicationWindow {
    id: root

    width: 800
    height: 600
    visible: true
    title: qsTr("Scrcpy Manager")

    color: "black"

    RowLayout {
        id: mainLayout

        anchors.fill: parent

        spacing: 10

        ListView {
            id: _list

            Layout.preferredWidth: parent.width * 0.5
            Layout.fillWidth: true
            Layout.fillHeight: true

            visible: !_scrcpyItem.visible
            enabled: !ScrcpyManager.currentlyConnecting
            opacity: enabled ? 1 : 0.5

            spacing: 10
            model: ScrcpyManager.devicesList
            delegate: Frame {
                width: parent.width
                height: 80

                background: Rectangle {
                    color: "lightgray"
                    border.color: "black"
                    radius: 10
                }

                RowLayout {
                    anchors.fill: parent
                    spacing: 10

                    Text {
                        text: modelData
                        font.pixelSize: 20
                        color: "black"
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Button {
                        text: "Connect"
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: {
                            ScrcpyManager.connectToDevice(_scrcpyItem,
                                                          modelData)
                        }
                    }
                }
            }
        }

        ColumnLayout {
            id: settingsContainer

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.5

            spacing: 10
            visible: !_scrcpyItem.visible

            RowLayout {
                Layout.fillWidth: true
                spacing: 5
                Label {
                    text: "Size"
                    Layout.alignment: Qt.AlignVCenter
                }
                ComboBox {
                    id: maxSizeComboBox
                    Layout.fillWidth: true
                    model: ScrcpyManager.maxSizeArray
                    currentIndex: ScrcpyManager.maxSizeIndex
                    onCurrentIndexChanged: {
                        ScrcpyManager.maxSizeIndex = currentIndex
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 5
                Label {
                    text: "Rate"
                    Layout.alignment: Qt.AlignVCenter
                }
                ComboBox {
                    id: bitRateUnitsComboBox
                    Layout.fillWidth: true
                    model: ScrcpyManager.availableBitRatesUnits
                    currentIndex: ScrcpyManager.bitRateUnits === "Mbps" ? 0 : 1
                    onCurrentIndexChanged: {
                        ScrcpyManager.bitRateUnits = (currentIndex === 0 ? "Mbps" : "Kbps")
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 5
                Label {
                    text: "Bits"
                    Layout.alignment: Qt.AlignVCenter
                }
                TextField {
                    id: bitRateField
                    Layout.fillWidth: true
                    placeholderText: "Bit Rate"
                    text: ScrcpyManager.bitRateNumeric.toString()
                    onTextChanged: {
                        ScrcpyManager.bitRateNumeric = parseInt(text)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 5
                Label {
                    text: "Orientation"
                    Layout.alignment: Qt.AlignVCenter
                }
                ComboBox {
                    id: lockOrientationComboBox
                    Layout.fillWidth: true
                    model: ScrcpyManager.availableOrientations
                    currentIndex: ScrcpyManager.lockOrientationIndex
                    onCurrentIndexChanged: {
                        ScrcpyManager.lockOrientationIndex = currentIndex
                    }
                }
            }

            CheckBox {
                id: autoUpdateDeviceCheck
                text: "Auto Update Device"
                checked: ScrcpyManager.autoUpdateDevice
                onToggled: {
                    ScrcpyManager.autoUpdateDevice = checked
                }
            }
        }
    }

    RowLayout {
        id: _controlsContainer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: 10
        visible: _scrcpyItem.visible

        Button {
            text: "Disconnect"
            onClicked: {
                ScrcpyManager.disconnectFromDevice()
            }
        }
    }

    ScrcpyItem {
        id: _scrcpyItem
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: _controlsContainer.top
        }
        visible: false

        onFrameSizeChanged: {
            if (visible) {
                root.width = Math.max(frameSize.width, 800)
                root.height = frameSize.height + _controlsContainer.height
            }
        }
    }

    Connections {
        target: ScrcpyManager

        onDeviceConnected: {
            _scrcpyItem.visible = true
            _scrcpyItem.focus = true
        }

        onDeviceDisconnected: {
            _scrcpyItem.visible = false
            _scrcpyItem.focus = false
        }
    }

    Component.onCompleted: {
        ScrcpyManager.listDevices()
    }
}
