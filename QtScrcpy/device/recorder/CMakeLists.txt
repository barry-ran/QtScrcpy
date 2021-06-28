set(QS_SOURCES_DEVICE_RECORDER
    recorder.h
    recorder.cpp
)

add_library(recorder ${QS_SOURCES_DEVICE_RECORDER})
target_include_directories(recorder PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${PROJECT_SOURCE_DIR}/third_party/ffmpeg/include"
    )
target_link_libraries(recorder PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    util
    )
