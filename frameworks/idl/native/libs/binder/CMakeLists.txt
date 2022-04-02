project(libidlbinder)

add_library(libidlbinder SHARED droid/os/IServiceManager.cpp)

target_include_directories(
        libidlbinder
        PRIVATE ${PROJECT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_SOURCE_DIR}/system/core/include
        PRIVATE ${CMAKE_SOURCE_DIR}/frameworks/idl/include
)

set_target_properties(libidlbinder PROPERTIES OUTPUT_NAME "idlbinder")

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

find_library(libutils libutils.so ${LIBRARY_OUTPUT_PATH}/system/core/libutils)
target_link_libraries(libidlbinder ${libutils})

find_library(liblog liblog.so ${LIBRARY_OUTPUT_PATH}/system/core/liblog)
target_link_libraries(libidlbinder ${liblog})

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}${relativePath}")

