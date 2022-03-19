project(libbinder)

add_library(libbinder SHARED Static.cpp ProcessState.cpp)
set_target_properties(libbinder PROPERTIES OUTPUT_NAME "binder")

target_include_directories(
        libbinder
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

find_library(libutils libutils.so ${LIBRARY_OUTPUT_PATH}/system/core/libutils)
target_link_libraries(libbinder ${libutils})

find_library(liblog liblog.so ${LIBRARY_OUTPUT_PATH}/system/core/liblog)
target_link_libraries(libbinder ${liblog})

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}${relativePath}")
