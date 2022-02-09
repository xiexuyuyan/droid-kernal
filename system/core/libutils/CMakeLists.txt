project(libutils)

file(GLOB_RECURSE sourceFiles ./*.cpp)
file(GLOB_RECURSE headerFiles ./include/*.h)

add_library(libutils_shared SHARED ${sourceFiles} ${headerFiles})
add_library(libutils_static STATIC ${sourceFiles} ${headerFiles})

# set build/lib 's output name
set_target_properties(libutils_shared PROPERTIES OUTPUT_NAME "utils")
set_target_properties(libutils_static PROPERTIES OUTPUT_NAME "utils")
set_target_properties(libutils_shared PROPERTIES VERSION 1.0 SOVERSION 1)

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
# build path
set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}${relativePath}")

# install path
set(CMAKE_INSTALL_PREFIX  "${CMAKE_INSTALL_PREFIX}${relativePath}")
# install lib
INSTALL(TARGETS libutils_shared libutils_static
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        )
# install include
INSTALL(FILES ${headerFiles} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
