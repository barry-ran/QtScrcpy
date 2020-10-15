HEADERS += \
    $$PWD/nativewindowutils.h \
    $$PWD/windowframelesshelper.h \
    $$PWD/windowframelessmanager.h

SOURCES += \
    $$PWD/windowframelesshelper.cpp \
    $$PWD/windowframelessmanager.cpp

win32 {
    HEADERS += $$PWD/windownativeeventfilterwin.h
    SOURCES += $$PWD/windownativeeventfilterwin.cpp \
        $$PWD/nativewindowutils.cpp \
}

mac {
    HEADERS += $$PWD/windownativeeventfiltermac.h
    SOURCES += $$PWD/windownativeeventfiltermac.mm \
        $$PWD/nativewindowutils.mm
}

linux {

}
