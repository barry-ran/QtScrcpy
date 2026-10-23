cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(all)

option(QTSCRCPY_BUILD_VISIBILITY_TESTS "Build window visibility UI tests" OFF)
if(QTSCRCPY_BUILD_VISIBILITY_TESTS)
    enable_testing()
endif()

add_subdirectory(QtScrcpy)
