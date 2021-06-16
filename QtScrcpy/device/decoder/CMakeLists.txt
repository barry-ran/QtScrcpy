set(QS_SOURCES_DEVICE_DECODER
    avframeconvert.h
    avframeconvert.cpp
    decoder.h
    decoder.cpp
    fpscounter.h
    fpscounter.cpp
    videobuffer.h
    videobuffer.cpp
)

add_library(decoder ${QS_SOURCES_DEVICE_DECODER})
target_include_directories(decoder PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${PROJECT_SOURCE_DIR}/third_party/ffmpeg/include"
    )
target_link_libraries(decoder PUBLIC ${QS_EXTERNAL_LIBS_FFMPEG})
target_link_libraries(decoder PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    util
    )
