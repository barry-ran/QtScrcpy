FORMS +=

HEADERS += \
    $$PWD/keepratiowidget.h \
    $$PWD/magneticwidget.h

SOURCES += \
    $$PWD/keepratiowidget.cpp \
    $$PWD/magneticwidget.cpp

win32 {
    SOURCES +=
}

mac {
    SOURCES +=
}

linux {
    SOURCES +=
}

include ($$PWD/windowframelesshelper/windowframelesshelper.pri)
