cmake_minimum_required(VERSION 3.19)
project(WindowVisibilityTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
enable_testing()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Test)

add_executable(windowvisibility_test
    windowvisibility_test.cpp
    ../../QtScrcpy/uibase/windowvisibilitycontroller.cpp
    ../../QtScrcpy/uibase/windowvisibilitycontroller.h
)
target_include_directories(windowvisibility_test PRIVATE ../../QtScrcpy/uibase)
target_link_libraries(windowvisibility_test PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Test)
if(WIN32)
    target_link_libraries(windowvisibility_test PRIVATE user32)
endif()
if(NOT MSVC)
    target_compile_options(windowvisibility_test PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

add_test(NAME windowvisibility COMMAND windowvisibility_test)
