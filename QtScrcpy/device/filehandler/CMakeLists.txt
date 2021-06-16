set(QS_SOURCES_DEVICE_FILEHANDLER
    filehandler.h
    filehandler.cpp
)

add_library(filehandler ${QS_SOURCES_DEVICE_FILEHANDLER})
target_include_directories(filehandler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(filehandler PUBLIC adb)
target_link_libraries(filehandler PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
