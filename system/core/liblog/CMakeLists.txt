project(liblog)

# add include directories to build
include_directories(BEFORE "include")

file(GLOB_RECURSE sourceFiles ./*.cpp)

add_library(liblog_shared SHARED ${sourceFiles})
# add_library(liblog_static STATIC ${sourceFiles})

# set build/lib 's output name
# set_target_properties(liblog_static PROPERTIES OUTPUT_NAME "log")
set_target_properties(liblog_shared PROPERTIES OUTPUT_NAME "log")
# set_target_properties(liblog_shared PROPERTIES VERSION 1.0 SOVERSION 1)

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
# build path
set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}${relativePath}")