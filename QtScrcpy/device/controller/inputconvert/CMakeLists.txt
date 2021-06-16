SET(QS_SUBDIRECTORIES_DEVICE_CONTROLLER_INPUTCONVERT
    keymap
)

set(QS_SOURCES_DEVICE_CONTROLLER_INPUTCONVERT
    controlmsg.h
    controlmsg.cpp
    inputconvertbase.h
    inputconvertbase.cpp
    inputconvertgame.h
    inputconvertgame.cpp
    inputconvertnormal.h
    inputconvertnormal.cpp
)

add_library(inputconvert ${QS_SOURCES_DEVICE_CONTROLLER_INPUTCONVERT})

target_include_directories(inputconvert PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

foreach(QS_SUBDIRECTORY_DEVICE_CONTROLLER_INPUTCONVERT ${QS_SUBDIRECTORIES_DEVICE_CONTROLLER_INPUTCONVERT})
    add_subdirectory (${QS_SUBDIRECTORY_DEVICE_CONTROLLER_INPUTCONVERT})
endforeach()

target_link_libraries(inputconvert PUBLIC
    common
    # controller
        android
    )
target_link_libraries(inputconvert INTERFACE
    # controller
        # inputconvert (self)
            keymap
    )
target_link_libraries(inputconvert PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets

    controller
    util
    )
