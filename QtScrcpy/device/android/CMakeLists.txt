set(QS_SOURCES_DEVICE_ANDROID
    input.h
    keycodes.h
)

add_library(android INTERFACE ${QS_SOURCES_DEVICE_ANDROID})
target_include_directories(android INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
