HEADERS += \
    $$PWD/mousetap.h

SOURCES += \
    $$PWD/mousetap.cpp

win32 {
    HEADERS += $$PWD/winmousetap.h
    SOURCES += $$PWD/winmousetap.cpp
    LIBS    += -lUser32
}

mac {
    HEADERS += $$PWD/cocoamousetap.h
    OBJECTIVE_SOURCES += $$PWD/cocoamousetap.mm
    LIBS += -framework Appkit
    QMAKE_CFLAGS += -mmacosx-version-min=10.6
}

linux {
    HEADERS += $$PWD/xmousetap.h
    SOURCES += $$PWD/xmousetap.cpp
    LIBS    += -lxcb
    QT      += x11extras
}
