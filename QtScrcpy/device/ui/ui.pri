SOURCES += \
    $$PWD/videoform.cpp \
    $$PWD/toolform.cpp

HEADERS += \
    $$PWD/videoform.h \
    $$PWD/toolform.h

FORMS += \
    $$PWD/videoform.ui \
    $$PWD/toolform.ui

#DEFINES += USE_QTQUICK

contains(DEFINES, USE_QTQUICK) {
    QT += quickwidgets
}
