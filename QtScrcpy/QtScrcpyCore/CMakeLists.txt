set(QSC_PROJECT_NAME "QtScrcpyCore")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Network REQUIRED)

# check arch
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(QSC_CPU_ARCH x64)
else()
    set(QSC_CPU_ARCH x86)
endif()
message(STATUS "[${PROJECT_NAME}] QSC_CPU_ARCH:${QSC_CPU_ARCH}")

if(NOT DEFINED QSC_DEPLOY_PATH)
    message(FATAL_ERROR "-- [QtScrcpyCore] need QSC_DEPLOY_PATH")
endif()

message("-- [QtScrcpyCore] QSC_DEPLOY_PATH: ${QSC_DEPLOY_PATH}")

#
# Sources
#

# adb
set(QSC_ADB_SOURCES
    src/adb/adbprocessimpl.h
    src/adb/adbprocessimpl.cpp
    src/adb/adbprocess.cpp
)
source_group(src/adb FILES ${QSC_ADB_SOURCES})

# common
set(QSC_COMMON_SOURCES
    src/common/qscrcpyevent.h
)
source_group(src/common FILES ${QSC_COMMON_SOURCES})

# include
set(QSC_INCLUDE_SOURCES
    include/QtScrcpyCore.h
    include/QtScrcpyCoreDef.h
    include/adbprocess.h
)
source_group(include FILES ${QSC_INCLUDE_SOURCES})

# device
set(QSC_DEVICE_SOURCES
    src/device/device.h
    src/device/device.cpp
    src/device/compat.h
    src/device/android/input.h
    src/device/android/keycodes.h
    src/device/controller/controller.h
    src/device/controller/controller.cpp
    src/device/controller/bufferutil.h
    src/device/controller/bufferutil.cpp
    src/device/controller/inputconvert/inputconvertbase.h
    src/device/controller/inputconvert/inputconvertbase.cpp
    src/device/controller/inputconvert/inputconvertnormal.h
    src/device/controller/inputconvert/inputconvertnormal.cpp
    src/device/controller/inputconvert/inputconvertgame.h
    src/device/controller/inputconvert/inputconvertgame.cpp
    src/device/controller/inputconvert/controlmsg.h
    src/device/controller/inputconvert/controlmsg.cpp
    src/device/controller/inputconvert/keymap/keymap.h
    src/device/controller/inputconvert/keymap/keymap.cpp
    src/device/controller/receiver/devicemsg.h
    src/device/controller/receiver/devicemsg.cpp
    src/device/controller/receiver/receiver.h
    src/device/controller/receiver/receiver.cpp
    src/device/decoder/avframeconvert.h
    src/device/decoder/avframeconvert.cpp
    src/device/decoder/decoder.h
    src/device/decoder/decoder.cpp
    src/device/decoder/fpscounter.h
    src/device/decoder/fpscounter.cpp
    src/device/decoder/videobuffer.h
    src/device/decoder/videobuffer.cpp
    src/device/filehandler/filehandler.h
    src/device/filehandler/filehandler.cpp
    src/device/recorder/recorder.h
    src/device/recorder/recorder.cpp
    src/device/server/server.h
    src/device/server/server.cpp
    src/device/server/tcpserver.h
    src/device/server/tcpserver.cpp
    src/device/server/videosocket.h
    src/device/server/videosocket.cpp
    src/device/stream/stream.h
    src/device/stream/stream.cpp
)
source_group(src/device FILES ${QSC_DEVICE_SOURCES})

# devicemanage
set(QSC_DEVICEMANAGE_SOURCES
    src/devicemanage/devicemanage.h
    src/devicemanage/devicemanage.cpp
)
source_group(src/devicemanage FILES ${QSC_DEVICEMANAGE_SOURCES})

add_library(QtScrcpyCore STATIC
  ${QSC_ADB_SOURCES}
  ${QSC_COMMON_SOURCES}
  ${QSC_INCLUDE_SOURCES}
  ${QSC_DEVICE_SOURCES}
  ${QSC_DEVICEMANAGE_SOURCES}
)

target_link_libraries(QtScrcpyCore PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

set_target_properties(${QSC_PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${QSC_DEPLOY_PATH}/$<0:>"
)

target_include_directories(${QSC_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/adb)
target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/device)
target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/device/filehandler)
target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/device/android)
target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/device/decoder)
target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/device/controller)
target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/device/controller/receiver)
target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/device/controller/inputconvert)
target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/device/controller/inputconvert/keymap)
target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/device/server)
target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/device/stream)
target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/device/ui)
target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/device/recorder)
target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/devicemanage)

#
# plantform deps
#

# windows
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # ffmpeg
    # include
    target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/ffmpeg/include)
    # link
    set(FFMPEG_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/ffmpeg/lib/${QSC_CPU_ARCH}")
    target_link_directories(${QSC_PROJECT_NAME} PUBLIC ${FFMPEG_LIB_PATH})
    target_link_libraries(${QSC_PROJECT_NAME} PRIVATE
        avformat
        avcodec
        avutil
        swscale
    )
    # copy
    set(THIRD_PARTY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party")
    set(FFMPEG_BIN_PATH "${THIRD_PARTY_PATH}/ffmpeg/bin/${QSC_CPU_ARCH}")
    add_custom_command(TARGET ${QSC_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FFMPEG_BIN_PATH}/avcodec-58.dll" "${QSC_DEPLOY_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FFMPEG_BIN_PATH}/avformat-58.dll" "${QSC_DEPLOY_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FFMPEG_BIN_PATH}/avutil-56.dll" "${QSC_DEPLOY_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FFMPEG_BIN_PATH}/swscale-5.dll" "${QSC_DEPLOY_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FFMPEG_BIN_PATH}/swresample-3.dll" "${QSC_DEPLOY_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${THIRD_PARTY_PATH}/adb/win/adb.exe" "${QSC_DEPLOY_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${THIRD_PARTY_PATH}/adb/win/AdbWinApi.dll" "${QSC_DEPLOY_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${THIRD_PARTY_PATH}/adb/win/AdbWinUsbApi.dll" "${QSC_DEPLOY_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${THIRD_PARTY_PATH}/scrcpy-server" "${QSC_DEPLOY_PATH}"
    )
endif()

# MacOS
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # ffmpeg
    # include
    target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/ffmpeg/include)
    # link
    set(FFMPEG_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/ffmpeg/lib")
    target_link_directories(${QSC_PROJECT_NAME} PUBLIC ${FFMPEG_LIB_PATH})
    target_link_libraries(${QSC_PROJECT_NAME} PRIVATE
        avformat.58
        avcodec.58
        avutil.56
        swscale.5
    )

    # copy bundle file
    add_custom_command(TARGET ${QSC_PROJECT_NAME} POST_BUILD
        # dylib,scrcpy-server,adb copy to Contents/MacOS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/ffmpeg/lib/libavcodec.58.dylib" "${QSC_DEPLOY_PATH}/MacOS"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/ffmpeg/lib/libavformat.58.dylib" "${QSC_DEPLOY_PATH}/MacOS"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/ffmpeg/lib/libavutil.56.dylib" "${QSC_DEPLOY_PATH}/MacOS"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/ffmpeg/lib/libswscale.5.dylib" "${QSC_DEPLOY_PATH}/MacOS"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/ffmpeg/lib/libswresample.3.dylib" "${QSC_DEPLOY_PATH}/MacOS"

        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/scrcpy-server" "${QSC_DEPLOY_PATH}/MacOS"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/adb/mac/adb" "${QSC_DEPLOY_PATH}/MacOS"
    )
endif()

# Linux
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # include
    target_include_directories(${QSC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/ffmpeg/include)
    # link
    set(FFMPEG_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/ffmpeg/lib")
    target_link_directories(${QSC_PROJECT_NAME} PUBLIC ${FFMPEG_LIB_PATH})
    target_link_libraries(${QSC_PROJECT_NAME} PRIVATE
        # ffmpeg
        avformat
        avcodec
        avutil
        swscale
    )

endif()
