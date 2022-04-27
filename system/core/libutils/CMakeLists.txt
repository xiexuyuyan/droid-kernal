project(libutils)

set(CMAKE_CXX_COMPILER  "/usr/bin/clang++" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_CXX_STANDARD 17)

# add include directories to build
include_directories(BEFORE "include")

file(GLOB_RECURSE sourceFiles ./*.cpp)

add_library(libutils_shared SHARED ${sourceFiles})
add_library(libutils_static STATIC ${sourceFiles})

# set build/lib 's output name
set_target_properties(libutils_shared PROPERTIES OUTPUT_NAME "utils")
set_target_properties(libutils_static PROPERTIES OUTPUT_NAME "utils")
set_target_properties(libutils_shared PROPERTIES VERSION 1.0 SOVERSION 1)

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
# build path
set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}${relativePath}")