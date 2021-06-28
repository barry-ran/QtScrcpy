set(QS_SOURCES_COMMON
    qscrcpyevent.h
)

add_library(common INTERFACE ${QS_SOURCES_COMMON})
target_include_directories(common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
