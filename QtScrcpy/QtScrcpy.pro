#-------------------------------------------------
#
# Project created by QtCreator 2018-10-07T12:36:10
#
#-------------------------------------------------

QT += core gui
QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtScrcpy
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#DEFINES += SKIP_FRAMES

# 源码
SOURCES += \
        main.cpp \
        dialog.cpp \
    videoform.cpp

HEADERS += \
        dialog.h \
    videoform.h

FORMS += \
        dialog.ui \
    videoform.ui

# ***********************************************************
# 输出目录
# ***********************************************************
# Win平台下输出目录
win32 {
    CONFIG(debug, debug|release) {
        DESTDIR = $$PWD/../output/win/debug
    } else {
        DESTDIR = $$PWD/../output/win/release
    }
}

# Mac os平台下输出目录
macos {
    CONFIG(debug, debug|release) {
        DESTDIR = $$PWD/../output/mac/debug
    } else {
        DESTDIR = $$PWD/../output/mac/release
    }
}

# 子工程
include ($$PWD/common/common.pri)
include ($$PWD/server/server.pri)
include ($$PWD/adb/adb.pri)
include ($$PWD/decoder/decoder.pri)
include ($$PWD/render/render.pri)
include ($$PWD/android/android.pri)
include ($$PWD/inputcontrol/inputcontrol.pri)

# 附加包含路径
INCLUDEPATH += \
        $$PWD/../third_party/ffmpeg/include \
        $$PWD/common \
        $$PWD/server \
        $$PWD/adb \
        $$PWD/decoder \
        $$PWD/render \
        $$PWD/android \
        $$PWD/inputcontrol

# ***********************************************************
# 依赖模块
# ***********************************************************
# Win平台下依赖模块
win32 {
    LIBS += \
            -L$$PWD/../third_party/ffmpeg/lib -lavcodec \
            -L$$PWD/../third_party/ffmpeg/lib -lavformat \
            -L$$PWD/../third_party/ffmpeg/lib -lavutil \
            -L$$PWD/../third_party/ffmpeg/lib -lswscale \
            -lUser32
}

# mac平台下依赖模块
macos {
    LIBS += \
            -L$$PWD/../third_party/ffmpeg/lib -lavcodec.58 \
            -L$$PWD/../third_party/ffmpeg/lib -lavformat.58 \
            -L$$PWD/../third_party/ffmpeg/lib -lavutil.56 \
            -L$$PWD/../third_party/ffmpeg/lib -lswscale.5

    APP_SCRCPY_SERVER.files = $$files($$PWD/../third_party/scrcpy-server.jar)
    APP_SCRCPY_SERVER.path = Contents/MacOS
    QMAKE_BUNDLE_DATA += APP_SCRCPY_SERVER

    APP_ADB.files = $$files($$PWD/../third_party/adb/adb)
    APP_ADB.path = Contents/MacOS
    QMAKE_BUNDLE_DATA += APP_ADB
}

RESOURCES += \
    res.qrc

