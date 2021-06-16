set(QS_SOURCES_ADB
    adbprocess.h
    adbprocess.cpp
)

add_library(adb ${QS_SOURCES_ADB})
target_include_directories(adb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(adb PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    util
    )
