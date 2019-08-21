HEADERS += \
    $$PWD/inputconvertbase.h \
    $$PWD/inputconvertgame.h \
    $$PWD/inputconvertnormal.h \
    $$PWD/controlmsg.h

SOURCES += \
    $$PWD/inputconvertbase.cpp \
    $$PWD/inputconvertgame.cpp \
    $$PWD/inputconvertnormal.cpp \
    $$PWD/controlmsg.cpp

include ($$PWD/keymap/keymap.pri)

INCLUDEPATH += \
        $$PWD/keymap

