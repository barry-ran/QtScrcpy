set(QS_SOURCES_UIBASE
    keepratiowidget.h
    keepratiowidget.cpp
    magneticwidget.h
    magneticwidget.cpp
)

add_library(uibase ${QS_SOURCES_UIBASE})
target_include_directories(uibase PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(uibase PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
