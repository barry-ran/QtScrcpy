set(QS_SOURCES_FONTAWESOME
    iconhelper.h
    iconhelper.cpp
)

add_library(fontawesome ${QS_SOURCES_FONTAWESOME})
target_include_directories(fontawesome PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fontawesome PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
