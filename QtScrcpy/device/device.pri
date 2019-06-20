HEADERS += \
    $$PWD/device.h

SOURCES += \
    $$PWD/device.cpp

include ($$PWD/server/server.pri)
include ($$PWD/decoder/decoder.pri)
include ($$PWD/render/render.pri)
include ($$PWD/stream/stream.pri)
include ($$PWD/android/android.pri)
include ($$PWD/controller/controller.pri)
include ($$PWD/filehandler/filehandler.pri)
include ($$PWD/recorder/recorder.pri)
include ($$PWD/ui/ui.pri)

INCLUDEPATH += \
        $$PWD/../../third_party/ffmpeg/include \
        $$PWD/server \
        $$PWD/decoder \
        $$PWD/render \
        $$PWD/stream \
        $$PWD/android \
        $$PWD/controller \
        $$PWD/filehandler \
        $$PWD/recorder \
        $$PWD/ui
