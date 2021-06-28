set(QS_SOURCES_DEVICEMANAGE
    devicemanage.h
    devicemanage.cpp
)

add_library(devicemanage ${QS_SOURCES_DEVICEMANAGE})
target_include_directories(devicemanage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(devicemanage PUBLIC device)
target_link_libraries(devicemanage PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
        server
        ui
    )
