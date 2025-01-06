qt_add_executable(appTestQml
    main.cpp
)

qt_add_qml_module(appTestQml
    URI TestQml
    VERSION 1.0
    QML_FILES
        Main.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appTestQml PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appTestQml
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appTestQml
    PRIVATE Qt6::Quick QtScrcpyQmlplugin
)

include(GNUInstallDirs)
install(TARGETS appTestQml
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(QC_CPU_ARCH x64)
else()
    set(QC_CPU_ARCH x86)
endif()

set_target_properties(appTestQml PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../output/${QC_CPU_ARCH}/${CMAKE_BUILD_TYPE}/$<0:>"
)

add_custom_command(TARGET appTestQml POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/../../config/config.ini" "${CMAKE_CURRENT_SOURCE_DIR}/../../output/${QC_CPU_ARCH}/${CMAKE_BUILD_TYPE}/config/config.ini"
)
