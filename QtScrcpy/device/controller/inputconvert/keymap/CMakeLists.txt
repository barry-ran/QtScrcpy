set(QS_SOURCES_DEVICE_CONTROLLER_INPUTCONVERT_KEYMAP
    keymap.h
    keymap.cpp
)

add_library(keymap ${QS_SOURCES_DEVICE_CONTROLLER_INPUTCONVERT_KEYMAP})
target_include_directories(keymap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(keymap PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
