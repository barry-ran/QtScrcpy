set(QS_SOURCES_DEVICE_RENDER
    qyuvopenglwidget.h
    qyuvopenglwidget.cpp
)

add_library(render ${QS_SOURCES_DEVICE_RENDER})
target_include_directories(render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(render PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
