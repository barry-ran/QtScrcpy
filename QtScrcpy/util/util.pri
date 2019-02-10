mac {
    HEADERS += $$PWD/cocoamousetap.h
    OBJECTIVE_SOURCES += $$PWD/cocoamousetap.mm
    LIBS += -framework Appkit
    QMAKE_CFLAGS += -mmacosx-version-min=10.6
}
