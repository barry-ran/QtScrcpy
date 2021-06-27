set(QS_SOURCES_DEVICE_SERVER
    server.h
    server.cpp
    tcpserver.h
    tcpserver.cpp
    videosocket.h
    videosocket.cpp
)

add_library(server ${QS_SOURCES_DEVICE_SERVER})
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(server PUBLIC
    Qt${QT_VERSION_MAJOR}::Network
    adb
    )
target_link_libraries(server PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    common
    util
    )
