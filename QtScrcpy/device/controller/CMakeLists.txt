SET(QS_SUBDIRECTORIES_DEVICE_CONTROLLER
    inputconvert
    receiver
)

SET(QS_SOURCES_DEVICE_CONTROLLER
    controller.h
    controller.cpp
)

add_library(controller ${QS_SOURCES_DEVICE_CONTROLLER})

target_include_directories(controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

foreach(QS_SUBDIRECTORY_DEVICE_CONTROLLER ${QS_SUBDIRECTORIES_DEVICE_CONTROLLER})
    add_subdirectory (${QS_SUBDIRECTORY_DEVICE_CONTROLLER})
endforeach()

target_link_libraries(controller PUBLIC
    inputconvert
    )
target_link_libraries(controller PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    receiver
    server
    )
