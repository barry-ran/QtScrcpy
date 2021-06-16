set(QS_SOURCES_DEVICE_STREAM
    stream.h
    stream.cpp
)

add_library(stream ${QS_SOURCES_DEVICE_STREAM})
target_include_directories(stream PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${PROJECT_SOURCE_DIR}/third_party/ffmpeg/include"
    )

target_link_libraries(stream PUBLIC ${QS_EXTERNAL_LIBS_FFMPEG})
target_link_libraries(stream PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    #controller
        decoder
        recorder
        server
    util
    )
