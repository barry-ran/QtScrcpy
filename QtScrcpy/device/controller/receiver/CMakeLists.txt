SET(QS_SOURCES_DEVICE_CONTROLLER_RECEIVER
    devicemsg.h
    devicemsg.cpp
    receiver.h
    receiver.cpp
    )

add_library(receiver ${QS_SOURCES_DEVICE_CONTROLLER_RECEIVER})
target_include_directories(receiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(receiver PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    util
    )
