set(QS_SUBDIRECTORIES_DEVICE
    android
    controller
    decoder
    filehandler
    recorder
    render
    server
    stream
    ui
)

set(QS_SOURCES_DEVICE
    device.h
    device.cpp
)

add_library(device ${QS_SOURCES_DEVICE})

target_include_directories(device PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

foreach(QS_SUBDIRECTORY_DEVICE ${QS_SUBDIRECTORIES_DEVICE})
    add_subdirectory (${QS_SUBDIRECTORY_DEVICE})
endforeach()

target_link_libraries(device INTERFACE inputconvert)
target_link_libraries(device PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    # device (self)
        controller
        decoder
        filehandler
        recorder
        server
        stream
        ui
    util
        mousetap
    )

