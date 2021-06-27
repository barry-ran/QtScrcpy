cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

# Read version numbers from file
file (STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/QtScrcpy/version STRING_VERSION)
message(STATUS "QtScrcpy Version ${STRING_VERSION}")
project(QtScrcpy
    VERSION ${STRING_VERSION}
    LANGUAGES C CXX
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    enable_language(OBJCXX)
endif()

# Split version numbers
string(REPLACE "." ";" VERSION_LIST ${STRING_VERSION})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_PATCH)

add_subdirectory(QtScrcpy)
