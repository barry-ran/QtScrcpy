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
msvc{
    QMAKE_CFLAGS += -source-charset:utf-8
    QMAKE_CXXFLAGS += -source-charset:utf-8
}

# warning as error
#4566 https://github.com/Chuyu-Team/VC-LTL/issues/27
*g++*: QMAKE_CXXFLAGS += -Werror
*msvc*: QMAKE_CXXFLAGS += /WX /wd4566

# run a server debugger and wait for a client to be attached
# DEFINES += SERVER_DEBUGGER
# select the debugger method ('old' for Android < 9, 'new' for Android >= 9)
# DEFINES += SERVER_DEBUGGER_METHOD_NEW

# 源码
SOURCES += \
        main.cpp \
        dialog.cpp

HEADERS += \
        dialog.h

FORMS += \
        dialog.ui

# 子工程
include ($$PWD/common/common.pri)
include ($$PWD/adb/adb.pri)
include ($$PWD/uibase/uibase.pri)
include ($$PWD/fontawesome/fontawesome.pri)
include ($$PWD/util/util.pri)
include ($$PWD/device/device.pri)
include ($$PWD/devicemanage/devicemanage.pri)

# 附加包含路径
INCLUDEPATH += \
        $$PWD/common \        
        $$PWD/adb \        
        $$PWD/uibase \        
        $$PWD/util \
        $$PWD/device \
        $$PWD/devicemanage \
        $$PWD/fontawesome

# 如果变量没有定义
# !defined(TEST_VAR, var) {
#     message("test")
# }

# 从文件读取版本号
CAT_VERSION = $$cat($$PWD/version)
# 拆分出版本号
VERSION_MAJOR = $$section(CAT_VERSION, ., 0, 0)
VERSION_MINOR = $$section(CAT_VERSION, ., 1, 1)
VERSION_PATCH = $$section(CAT_VERSION, ., 2, 2)
message("version:" $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_PATCH})

# qmake变量的方式定义版本号
VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_PATCH}

# ***********************************************************
# Win平台下配置
# ***********************************************************
win32 {
    # 通过rc的方式的话，VERSION变量rc中获取不到,定义为宏方便rc中使用
    DEFINES += VERSION_MAJOR=$${VERSION_MAJOR}
    DEFINES += VERSION_MINOR=$${VERSION_MINOR}
    DEFINES += VERSION_PATCH=$${VERSION_PATCH}
    DEFINES += VERSION_RC_STR=\\\"$${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_PATCH}\\\"

    contains(QT_ARCH, x86_64) {
        message("x64")
        # 输出目录
        CONFIG(debug, debug|release) {
            DESTDIR = $$PWD/../output/win/x64/debug
        } else {
            DESTDIR = $$PWD/../output/win/x64/release
        }

        # 依赖模块
        LIBS += \
                -L$$PWD/../third_party/ffmpeg/lib/x64 -lavformat \
                -L$$PWD/../third_party/ffmpeg/lib/x64 -lavcodec \
                -L$$PWD/../third_party/ffmpeg/lib/x64 -lavutil \
                -L$$PWD/../third_party/ffmpeg/lib/x64 -lswscale

        WIN_FFMPEG_SRC = $$PWD/../third_party/ffmpeg/bin/x64/*.dll
    } else {
        message("x86")
        # 输出目录
        CONFIG(debug, debug|release) {
            DESTDIR = $$PWD/../output/win/x86/debug
        } else {
            DESTDIR = $$PWD/../output/win/x86/release
        }

        # 依赖模块
        LIBS += \
                -L$$PWD/../third_party/ffmpeg/lib/x86 -lavformat \
                -L$$PWD/../third_party/ffmpeg/lib/x86 -lavcodec \
                -L$$PWD/../third_party/ffmpeg/lib/x86 -lavutil \
                -L$$PWD/../third_party/ffmpeg/lib/x86 -lswscale

        WIN_FFMPEG_SRC = $$PWD/../third_party/ffmpeg/bin/x86/*.dll
    }

    # 复制依赖库
    WIN_DST = $$DESTDIR

    WIN_FFMPEG_SRC ~= s,/,\\,g
    WIN_DST ~= s,/,\\,g

    QMAKE_POST_LINK += $$quote($$QMAKE_COPY $$WIN_FFMPEG_SRC $$WIN_DST$$escape_expand(\n\t))

    # windows rc file
    RC_FILE = $$PWD/res/QtScrcpy.rc
}

# ***********************************************************
# Mac平台下配置
# ***********************************************************
macos {
    # 输出目录
    CONFIG(debug, debug|release) {
        DESTDIR = $$PWD/../output/mac/debug
    } else {
        DESTDIR = $$PWD/../output/mac/release
    }

    # 依赖模块
    LIBS += \
            -L$$PWD/../third_party/ffmpeg/lib -lavformat.58 \
            -L$$PWD/../third_party/ffmpeg/lib -lavcodec.58 \
            -L$$PWD/../third_party/ffmpeg/lib -lavutil.56 \
            -L$$PWD/../third_party/ffmpeg/lib -lswscale.5

    # mac bundle file
    APP_SCRCPY_SERVER.files = $$files($$PWD/../third_party/scrcpy-server)
    APP_SCRCPY_SERVER.path = Contents/MacOS
    QMAKE_BUNDLE_DATA += APP_SCRCPY_SERVER

    APP_ADB.files = $$files($$PWD/../third_party/adb/mac/adb)
    APP_ADB.path = Contents/MacOS
    QMAKE_BUNDLE_DATA += APP_ADB

    APP_FFMPEG.files = $$files($$PWD/../third_party/ffmpeg/lib/*.dylib)
    APP_FFMPEG.path = Contents/MacOS
    QMAKE_BUNDLE_DATA += APP_FFMPEG

    APP_CONFIG.files = $$files($$PWD/../config/config.ini)
    APP_CONFIG.path = Contents/MacOS/config
    QMAKE_BUNDLE_DATA += APP_CONFIG

    # mac application icon
    ICON = $$PWD/res/QtScrcpy.icns
    QMAKE_INFO_PLIST = $$PWD/res/Info_Mac.plist

    # 定义目标命令（修改版本号字段）
    plistupdate.commands = /usr/libexec/PlistBuddy -c \"Set :CFBundleShortVersionString $$VERSION\" \
    -c \"Set :CFBundleVersion $$VERSION\" \
    $$DESTDIR/$${TARGET}.app/Contents/Info.plist

    # 增加额外目标
    QMAKE_EXTRA_TARGETS += plistupdate
    # 设置为前置依赖
    PRE_TARGETDEPS += plistupdate
}

# ***********************************************************
# Linux平台下配置
# ***********************************************************
linux {
    # 输出目录
    CONFIG(debug, debug|release) {
        DESTDIR = $$PWD/../output/linux/debug
    } else {
        DESTDIR = $$PWD/../output/linux/release
    }

    # 依赖模块
    LIBS += \
            -L$$PWD/../third_party/ffmpeg/lib -lavformat \
            -L$$PWD/../third_party/ffmpeg/lib -lavcodec \
            -L$$PWD/../third_party/ffmpeg/lib -lavutil \
            -L$$PWD/../third_party/ffmpeg/lib -lswscale

    # linux set app icon: https://blog.csdn.net/MrNoboday/article/details/82870853
}

# message("test")

RESOURCES += \
    res/res.qrc

