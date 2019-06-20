HEADERS += \
    $$PWD/controller.h

SOURCES += \
    $$PWD/controller.cpp

include ($$PWD/receiver/receiver.pri)
include ($$PWD/inputconvert/inputconvert.pri)

INCLUDEPATH += \
        $$PWD/receiver \
        $$PWD/inputconvert


