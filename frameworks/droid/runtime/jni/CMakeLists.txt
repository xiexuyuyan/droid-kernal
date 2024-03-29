project(libdroid_runtime)

find_package(JNI REQUIRED)

add_library(libdroid_runtime SHARED droid_util_Log.cpp)

set_target_properties(libdroid_runtime PROPERTIES OUTPUT_NAME "droid_runtime")

target_include_directories(
        libdroid_runtime
        PRIVATE ${JNI_INCLUDE_DIRS}
        PRIVATE ${CMAKE_SOURCE_DIR}/system/core/include
)

target_link_libraries(
        libdroid_runtime
        ${JNI_LIBRARIES}
)

if (EXISTS "${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so")
    message(STATUS "Project: when cmake build, there exists /system/core/liblog/liblog.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so "")
endif ()

find_library(liblog liblog.so ${LIBRARY_OUTPUT_PATH}/system/core/liblog)
target_link_libraries(libdroid_runtime ${libutils} ${liblog})

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}${relativePath}")