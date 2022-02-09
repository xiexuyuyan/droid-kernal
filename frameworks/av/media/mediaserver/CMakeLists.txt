project(mediaserver)

file(GLOB_RECURSE sourceFiles ./*.cpp)

add_executable(MediaServer main_mediaserver.cpp)

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH  ${EXECUTABLE_OUTPUT_PATH}${relativePath})

TARGET_INCLUDE_DIRECTORIES(
        MediaServer
        PRIVATE ${CMAKE_SOURCE_DIR}/system/core/libutils/include
        PRIVATE ${CMAKE_SOURCE_DIR}/system/core/liblog/include
)

if (EXISTS "${LIBRARY_OUTPUT_PATH}/system/core/libutils/libutils.so")
    message(STATUS "when cmake build, there exists /system/core/libutils/libutils.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/system/core/libutils/libutils.so "")
endif ()

if (EXISTS "${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so")
    message(STATUS "when cmake build, there exists /system/core/liblog/liblog.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so "")
endif ()

FIND_LIBRARY(libutils libutils.so ${LIBRARY_OUTPUT_PATH}/system/core/libutils)
FIND_LIBRARY(liblog liblog.so ${LIBRARY_OUTPUT_PATH}/system/core/liblog)
TARGET_LINK_LIBRARIES(MediaServer ${libutils} ${liblog})

# install path
set(CMAKE_INSTALL_PREFIX  "${CMAKE_INSTALL_PREFIX}${relativePath}")
# install bin
INSTALL(TARGETS MediaServer
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        )


#[[
message(STATUS "CMAKE_HOME_DIRECTORY is ${CMAKE_HOME_DIRECTORY}")
message(STATUS "PROJECT_SOURCE_DIR is ${PROJECT_SOURCE_DIR}")
message(STATUS "CMAKE_SOURCE_DIR is ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_INSTALL_PREFIX is ${CMAKE_INSTALL_PREFIX}")
message(STATUS "on execute pwd in libutils")
execute_process(COMMAND pwd)
]]
