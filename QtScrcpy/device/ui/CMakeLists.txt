set(QS_SOURCES_DEVICE_UI
    toolform.h
    toolform.cpp
    toolform.ui
    videoform.h
    videoform.cpp
    videoform.ui
)

add_library(ui ${QS_SOURCES_DEVICE_UI})
target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ui PRIVATE "${PROJECT_SOURCE_DIR}/third_party/ffmpeg/include")
target_link_libraries(ui PUBLIC
    device
    uibase
    )
target_link_libraries(ui PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    controller
        render
    fontawesome
    util
    )
