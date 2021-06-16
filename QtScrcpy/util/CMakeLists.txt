set(QS_SUBDIRECTORIES_UTIL
    mousetap
)

set(QS_SOURCES_UTIL
    bufferutil.h
    bufferutil.cpp
    compat.h
    config.h
    config.cpp
)

add_library(util ${QS_SOURCES_UTIL})

target_include_directories(util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

foreach(QS_SUBDIRECTORY_UTIL ${QS_SUBDIRECTORIES_UTIL})
    add_subdirectory (${QS_SUBDIRECTORY_UTIL})
endforeach()

target_link_libraries(util PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
