FORMS +=

HEADERS += \
    $$PWD/keepratiowidget.h \
    $$PWD/magneticwidget.h \
    $$PWD/windowframelesshelper.h \
    $$PWD/windownativeeventfilter.h


SOURCES += \
    $$PWD/keepratiowidget.cpp \
    $$PWD/magneticwidget.cpp \
    $$PWD/windownativeeventfilter.cpp

win32 {
    SOURCES += $$PWD/windowframelesshelper.cpp
}

mac {
    SOURCES += $$PWD/windowframelesshelper.mm
}

linux {
    SOURCES += $$PWD/windowframelesshelper.cpp
}
