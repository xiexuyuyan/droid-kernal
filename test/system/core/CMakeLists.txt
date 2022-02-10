project(LightPointerTest)

file(GLOB_RECURSE sourceFiles ./*.cpp)

add_executable(LightPointerTest LightPointer.cpp)

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH  ${EXECUTABLE_OUTPUT_PATH}${relativePath})

TARGET_INCLUDE_DIRECTORIES(
        LightPointerTest
        PRIVATE ${CMAKE_SOURCE_DIR}/system/core/include
)

if (EXISTS "${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so")
    message(STATUS "Project: when cmake build, there exists /system/core/liblog/liblog.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so "")
endif ()

FIND_LIBRARY(liblog liblog.so ${LIBRARY_OUTPUT_PATH}/system/core/liblog)
TARGET_LINK_LIBRARIES(LightPointerTest ${liblog})