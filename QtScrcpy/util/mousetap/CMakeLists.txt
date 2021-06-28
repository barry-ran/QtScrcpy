set(QS_SOURCES_UTIL_MOUSETAP
    mousetap.h
    mousetap.cpp
)

# Microsoft Windows
if(WIN32)

    list(APPEND QS_SOURCES_UTIL_MOUSETAP
        winmousetap.h
        winmousetap.cpp
    )
    set(QS_EXTERNAL_LIBS_UTIL_MOUSETAP User32)

elseif(UNIX)
# macOS
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        
        find_library(APPKIT AppKit REQUIRED)
        set(QS_EXTERNAL_LIBS_UTIL_MOUSETAP ${APPKIT})
        
        list(APPEND QS_SOURCES_UTIL_MOUSETAP
            cocoamousetap.h
            cocoamousetap.mm
        )

        target_compile_options(mousetap "-mmacosx-version-min=10.6")

    # Linux, BSD, etc.
    else()
        
        find_package(QT NAMES Qt6 Qt5 COMPONENTS X11Extras REQUIRED)
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS X11Extras REQUIRED)
        set(QS_EXTERNAL_LIBS_UTIL_MOUSETAP
            Qt${QT_VERSION_MAJOR}::X11Extras
            xcb
        )

        list(APPEND QS_SOURCES_UTIL_MOUSETAP
            xmousetap.h
            xmousetap.cpp
        )

    endif()
endif()

add_library(mousetap ${QS_SOURCES_UTIL_MOUSETAP})
target_link_libraries(mousetap PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    ${QS_EXTERNAL_LIBS_UTIL_MOUSETAP})
