project(OpenLogDriver)

add_executable(OpenLogDriver OpenLogDriver.cpp)
add_executable(LogPrinter LogPrinter.cpp)

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH  ${EXECUTABLE_OUTPUT_PATH}${relativePath})

TARGET_INCLUDE_DIRECTORIES(
        OpenLogDriver
        PRIVATE ${CMAKE_SOURCE_DIR}/system/core/include
)
TARGET_INCLUDE_DIRECTORIES(
        LogPrinter
        PRIVATE ${CMAKE_SOURCE_DIR}/system/core/include
)

if (EXISTS "${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so")
    message(STATUS "Project: when cmake build, there exists /system/core/liblog/liblog.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so "")
endif ()

if (EXISTS "${LIBRARY_OUTPUT_PATH}/system/core/libutils/libutils.so")
    message(STATUS "Project: when cmake build, there exists /system/core/libutils/libutils.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/system/core/libutils/libutils.so "")
endif ()

FIND_LIBRARY(liblog liblog.so ${LIBRARY_OUTPUT_PATH}/system/core/liblog)
FIND_LIBRARY(libutils libutils.so ${LIBRARY_OUTPUT_PATH}/system/core/libutils)
TARGET_LINK_LIBRARIES(OpenLogDriver ${liblog} ${libutils})
TARGET_LINK_LIBRARIES(LogPrinter ${liblog} ${libutils})