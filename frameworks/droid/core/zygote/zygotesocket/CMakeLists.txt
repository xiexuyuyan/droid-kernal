project(core_zygotesocket)

add_library(libcore_zygotesocket SHARED ZygoteSocketServer.cpp ZygoteSocketClient.cpp)
set_target_properties(libcore_zygotesocket PROPERTIES OUTPUT_NAME "core_zygotesocket")

target_include_directories(
        libcore_zygotesocket
        PRIVATE ${CMAKE_SOURCE_DIR}/frameworks/droid/core/include
        PRIVATE ${CMAKE_SOURCE_DIR}/system/core/include
)

if (EXISTS "${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so")
    message(STATUS "Project: when cmake build, there exists /system/core/liblog/liblog.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so "")
endif ()

find_library(liblog liblog.so ${LIBRARY_OUTPUT_PATH}/system/core/liblog)
target_link_libraries(libcore_zygotesocket ${liblog})

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}${relativePath}")
