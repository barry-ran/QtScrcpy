# For VS2019 and Xcode 12+ support.
cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

#
# Global config
#

# QC is "Qt CMake"
# https://www.kdab.com/wp-content/uploads/stories/QTVTC20-Using-Modern-CMake-Kevin-Funk.pdf

# QC Custom config
set(QC_PROJECT_NAME "QtScrcpy")
# Read version numbers from file
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/version QC_FILE_VERSION)
set(QC_PROJECT_VERSION ${QC_FILE_VERSION})

# Project declare
project(${QC_PROJECT_NAME} VERSION ${QC_PROJECT_VERSION} LANGUAGES CXX)
message(STATUS "[${PROJECT_NAME}] Project ${PROJECT_NAME} ${PROJECT_VERSION}")

# QC define

# check arch
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(QC_CPU_ARCH x64)
else()
    set(QC_CPU_ARCH x86)
endif()
message(STATUS "[${PROJECT_NAME}] CPU_ARCH:${QC_CPU_ARCH}")

# CMake set
#set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# default RelWithDebInfo
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
message(STATUS "[${PROJECT_NAME}] BUILD_TYPE:${CMAKE_BUILD_TYPE}")

# Compiler set
message(STATUS "[${PROJECT_NAME}] C++ compiler ID is: ${CMAKE_CXX_COMPILER_ID}")
if (MSVC)
    # FFmpeg cannot be compiled natively by MSVC version < 12.0 (2013)
    if(MSVC_VERSION LESS 1800)
        message(FATAL_ERROR "[${PROJECT_NAME}] ERROR: MSVC version is older than 12.0 (2013).")
    endif()

    message(STATUS "[${PROJECT_NAME}] Set Warnings as error")
    # warning level 3 and all warnings as errors
    add_compile_options(/W3 /WX /wd4566)

    # avoid warning C4819
    add_compile_options(-source-charset:utf-8)
    #add_compile_options(/utf-8)

    # ensure we use minimal "windows.h" lib without the crazy min max macros
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
    
    # disable SAFESEH - avoid "LNK2026: module unsafe"(Qt5.15&&vs2019)     
    add_link_options(/SAFESEH:NO)
endif()

if (NOT MSVC)
    message(STATUS "[${PROJECT_NAME}] Set warnings as error")
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror)

    # disable some warning
    add_compile_options(-Wno-nested-anon-types -Wno-c++17-extensions)
endif()

#
# Qt
#

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Network LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Network LinguistTools REQUIRED)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(QT NAMES Qt6 Qt5 COMPONENTS X11Extras REQUIRED)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS X11Extras REQUIRED)
endif()

message(STATUS "[${PROJECT_NAME}] Qt version is: ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}")

#
# Sources
#

# adb
set(QC_ADB_SOURCES
    adb/adbprocess.h
    adb/adbprocess.cpp
)
source_group(adb FILES ${QC_ADB_SOURCES})

# common
set(QC_COMMON_SOURCES
    common/qscrcpyevent.h
)
source_group(common FILES ${QC_COMMON_SOURCES})

# device
set(QC_DEVICE_SOURCES
    device/device.h
    device/device.cpp
    device/android/input.h
    device/android/keycodes.h
    device/controller/controller.h
    device/controller/controller.cpp
    device/controller/inputconvert/inputconvertbase.h
    device/controller/inputconvert/inputconvertbase.cpp
    device/controller/inputconvert/inputconvertnormal.h
    device/controller/inputconvert/inputconvertnormal.cpp
    device/controller/inputconvert/inputconvertgame.h
    device/controller/inputconvert/inputconvertgame.cpp
    device/controller/inputconvert/controlmsg.h
    device/controller/inputconvert/controlmsg.cpp
    device/controller/inputconvert/keymap/keymap.h
    device/controller/inputconvert/keymap/keymap.cpp
    device/controller/receiver/devicemsg.h
    device/controller/receiver/devicemsg.cpp
    device/controller/receiver/receiver.h
    device/controller/receiver/receiver.cpp
    device/decoder/avframeconvert.h
    device/decoder/avframeconvert.cpp
    device/decoder/decoder.h
    device/decoder/decoder.cpp
    device/decoder/fpscounter.h
    device/decoder/fpscounter.cpp
    device/decoder/videobuffer.h
    device/decoder/videobuffer.cpp
    device/filehandler/filehandler.h
    device/filehandler/filehandler.cpp
    device/recorder/recorder.h
    device/recorder/recorder.cpp
    device/render/qyuvopenglwidget.h
    device/render/qyuvopenglwidget.cpp
    device/server/server.h
    device/server/server.cpp
    device/server/tcpserver.h
    device/server/tcpserver.cpp
    device/server/videosocket.h
    device/server/videosocket.cpp
    device/stream/stream.h
    device/stream/stream.cpp
    device/ui/toolform.h
    device/ui/toolform.cpp
    device/ui/toolform.ui
    device/ui/videoform.h
    device/ui/videoform.cpp
    device/ui/videoform.ui
)
source_group(device FILES ${QC_DEVICE_SOURCES})

# devicemanage
set(QC_DEVICEMANAGE_SOURCES
    devicemanage/devicemanage.h
    devicemanage/devicemanage.cpp
)
source_group(devicemanage FILES ${QC_DEVICEMANAGE_SOURCES})

# fontawesome
set(QC_FONTAWESOME_SOURCES
    fontawesome/iconhelper.h
    fontawesome/iconhelper.cpp
)
source_group(fontawesome FILES ${QC_FONTAWESOME_SOURCES})

# uibase
set(QC_UIBASE_SOURCES
    uibase/keepratiowidget.h
    uibase/keepratiowidget.cpp
    uibase/magneticwidget.h
    uibase/magneticwidget.cpp
)
source_group(uibase FILES ${QC_UIBASE_SOURCES})

# util
set(QC_UTIL_SOURCES
    util/compat.h
    util/config.h
    util/config.cpp
    util/bufferutil.h
    util/bufferutil.cpp
    util/mousetap/mousetap.h
    util/mousetap/mousetap.cpp
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(QC_UTIL_SOURCES ${QC_UTIL_SOURCES} 
        util/mousetap/winmousetap.h
        util/mousetap/winmousetap.cpp
    )
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(QC_UTIL_SOURCES ${QC_UTIL_SOURCES} 
        util/mousetap/xmousetap.h
        util/mousetap/xmousetap.cpp
    )
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(QC_UTIL_SOURCES ${QC_UTIL_SOURCES} 
        util/mousetap/cocoamousetap.h
        util/mousetap/cocoamousetap.mm
    )
endif()
source_group(util FILES ${QC_UTIL_SOURCES})

# qrc
set(QC_QRC_SOURCES "res/res.qrc")

# main
set(QC_MAIN_SOURCES
    main.cpp
    dialog.cpp
    dialog.h
    dialog.ui
    ${QC_QRC_SOURCES}
)

# plantform file
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Define VERSION macros for .rc file
    add_compile_definitions(
        VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        VERSION_MINOR=${PROJECT_VERSION_MINOR}
        VERSION_PATCH=${PROJECT_VERSION_PATCH}
        VERSION_RC_STR="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
    )
    set(QC_PLANTFORM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/res/${PROJECT_NAME}.rc"
    )
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Step 1. add icns to source file, for MACOSX_PACKAGE_LOCATION copy
    set(QC_PLANTFORM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/res/${PROJECT_NAME}.icns"
    )
endif()

# 使用qt5_add_translation 根据已有ts文件生成qm文件，不用qt5_create_translation
# 感兴趣可以了解下qt5_create_translation用法 https://www.cnblogs.com/apocelipes/p/14355460.html
set(QC_TS_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/res/i18n/zh_CN.ts 
    ${CMAKE_CURRENT_SOURCE_DIR}/res/i18n/en_US.ts
)
set_source_files_properties(${QC_TS_FILES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/res/i18n")
qt5_add_translation(QC_QM_FILES ${QC_TS_FILES})

# all sources
set(QC_PROJECT_SOURCES
    ${QC_ADB_SOURCES}
    ${QC_COMMON_SOURCES}
    ${QC_DEVICE_SOURCES}
    ${QC_DEVICEMANAGE_SOURCES}
    ${QC_FONTAWESOME_SOURCES}
    ${QC_UIBASE_SOURCES}
    ${QC_UTIL_SOURCES}
    ${QC_MAIN_SOURCES}
    ${QC_PLANTFORM_SOURCES}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(QC_RUNTIME_TYPE MACOSX_BUNDLE)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(QC_RUNTIME_TYPE WIN32)
endif()


add_executable(${PROJECT_NAME} ${QC_RUNTIME_TYPE} ${QC_PROJECT_SOURCES})

#
# Internal include path (todo: remove this, use absolute path include)
#

target_include_directories(${PROJECT_NAME} PRIVATE adb)
target_include_directories(${PROJECT_NAME} PRIVATE common)
target_include_directories(${PROJECT_NAME} PRIVATE device)
target_include_directories(${PROJECT_NAME} PRIVATE device/filehandler)
target_include_directories(${PROJECT_NAME} PRIVATE device/android)
target_include_directories(${PROJECT_NAME} PRIVATE device/decoder)
target_include_directories(${PROJECT_NAME} PRIVATE device/controller)
target_include_directories(${PROJECT_NAME} PRIVATE device/controller/receiver)
target_include_directories(${PROJECT_NAME} PRIVATE device/controller/inputconvert)
target_include_directories(${PROJECT_NAME} PRIVATE device/controller/inputconvert/keymap)
target_include_directories(${PROJECT_NAME} PRIVATE device/server)
target_include_directories(${PROJECT_NAME} PRIVATE device/stream)
target_include_directories(${PROJECT_NAME} PRIVATE device/render)
target_include_directories(${PROJECT_NAME} PRIVATE device/ui)
target_include_directories(${PROJECT_NAME} PRIVATE device/recorder)
target_include_directories(${PROJECT_NAME} PRIVATE devicemanage)
target_include_directories(${PROJECT_NAME} PRIVATE fontawesome)
target_include_directories(${PROJECT_NAME} PRIVATE util)
target_include_directories(${PROJECT_NAME} PRIVATE uibase)

#
# common deps
#

# Qt
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

# output dir
# https://cmake.org/cmake/help/latest/prop_gbl/GENERATOR_IS_MULTI_CONFIG.html
get_property(QC_IS_MUTIL_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
message(STATUS "multi config:" QC_IS_MUTIL_CONFIG)

# $<0:> 使用生成器表达式为每个config设置RUNTIME_OUTPUT_DIRECTORY，这样multi config就不会自动追加CMAKE_BUILD_TYPE子目录了
# 1. multi config介绍 https://cmake.org/cmake/help/latest/prop_gbl/GENERATOR_IS_MULTI_CONFIG.html
# 2. multi config在不用表达式生成器时自动追加子目录说明 https://cmake.org/cmake/help/latest/prop_tgt/RUNTIME_OUTPUT_DIRECTORY.html
# 3. 使用表达式生成器禁止multi config自动追加子目录解决方案 https://stackoverflow.com/questions/7747857/in-cmake-how-do-i-work-around-the-debug-and-release-directories-visual-studio-2
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../output/${QC_CPU_ARCH}/${CMAKE_BUILD_TYPE}/$<0:>"
)

#
# plantform deps
#

# windows
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # ffmpeg
    # include
    target_include_directories(${PROJECT_NAME} PRIVATE ../third_party/ffmpeg/include)
    # link
    set(FFMPEG_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/ffmpeg/lib/${QC_CPU_ARCH}")
    target_link_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_LIB_PATH})
    target_link_libraries(${PROJECT_NAME} PRIVATE
        avformat
        avcodec
        avutil
        swscale
    )
    # copy
    set(FFMPEG_BIN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/ffmpeg/bin/${QC_CPU_ARCH}")
    get_target_property(FFMPEG_BIN_OUTPUT_PATH ${PROJECT_NAME} RUNTIME_OUTPUT_DIRECTORY)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FFMPEG_BIN_PATH}/avcodec-58.dll" "${FFMPEG_BIN_OUTPUT_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FFMPEG_BIN_PATH}/avformat-58.dll" "${FFMPEG_BIN_OUTPUT_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FFMPEG_BIN_PATH}/avutil-56.dll" "${FFMPEG_BIN_OUTPUT_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FFMPEG_BIN_PATH}/swscale-5.dll" "${FFMPEG_BIN_OUTPUT_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FFMPEG_BIN_PATH}/swresample-3.dll" "${FFMPEG_BIN_OUTPUT_PATH}"
    )
endif()

# MacOS
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # ffmpeg
    # include
    target_include_directories(${PROJECT_NAME} PRIVATE ../third_party/ffmpeg/include)
    # link
    set(FFMPEG_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/ffmpeg/lib")
    target_link_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_LIB_PATH})
    target_link_libraries(${PROJECT_NAME} PRIVATE
        avformat.58
        avcodec.58
        avutil.56
        swscale.5
    )

    # copy bundle file
    get_target_property(MACOS_BUNDLE_PATH ${PROJECT_NAME} RUNTIME_OUTPUT_DIRECTORY)
    set(MACOS_BUNDLE_PATH ${MACOS_BUNDLE_PATH}/${PROJECT_NAME}.app/Contents)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        # dylib,scrcpy-server,adb copy to Contents/MacOS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/ffmpeg/lib/libavcodec.58.dylib" "${MACOS_BUNDLE_PATH}/MacOS"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/ffmpeg/lib/libavformat.58.dylib" "${MACOS_BUNDLE_PATH}/MacOS"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/ffmpeg/lib/libavutil.56.dylib" "${MACOS_BUNDLE_PATH}/MacOS"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/ffmpeg/lib/libswscale.5.dylib" "${MACOS_BUNDLE_PATH}/MacOS"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/ffmpeg/lib/libswresample.3.dylib" "${MACOS_BUNDLE_PATH}/MacOS"

        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/scrcpy-server" "${MACOS_BUNDLE_PATH}/MacOS"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/adb/mac/adb" "${MACOS_BUNDLE_PATH}/MacOS"
        # config file copy to Contents/MacOS/config
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/../config/config.ini" "${MACOS_BUNDLE_PATH}/MacOS/config"
    )

    # Step 2. ues MACOSX_PACKAGE_LOCATION copy icns to Resources
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/res/${PROJECT_NAME}.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources
    )

    # use MACOSX_BUNDLE_INFO_PLIST custom plist, not use MACOSX_BUNDLE_BUNDLE_NAME etc..
    set(INFO_PLIST_TEMPLATE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/Info_Mac.plist.in")
    set(INFO_PLIST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/Info_Mac.plist")
    file(READ "${INFO_PLIST_TEMPLATE_FILE}" plist_contents)
    string(REPLACE "\${BUNDLE_VERSION}" "${PROJECT_VERSION}" plist_contents ${plist_contents})
    file(WRITE ${INFO_PLIST_FILE} ${plist_contents})
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${INFO_PLIST_FILE}"
        # "" disable code sign
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
    )

    # mac framework
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework AppKit")
endif()

# Linux
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)

    # include
    target_include_directories(${PROJECT_NAME} PRIVATE ../third_party/ffmpeg/include)
    # link
    set(FFMPEG_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/ffmpeg/lib")
    target_link_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_LIB_PATH})
    target_link_libraries(${PROJECT_NAME} PRIVATE
        # ffmpeg
        avformat
        avcodec
        avutil
        swscale
        # qx11
        Qt${QT_VERSION_MAJOR}::X11Extras
        # xcb https://doc.qt.io/qt-5/linux-requirements.html
        xcb
        # pthread
        Threads::Threads
    )

    # linux set app icon: https://blog.csdn.net/MrNoboday/article/details/82870853
endif()