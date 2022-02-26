project(libdroid_zygote)

find_package(JNI REQUIRED)

add_library(libdroid_zygote SHARED
        droid_internal_os_ZygoteInit.cpp
        droid_internal_os_ZygoteSocketServer.cpp
        droid_internal_os_ZygoteSocketClient.cpp
        )

set_target_properties(libdroid_zygote PROPERTIES OUTPUT_NAME "droid_zygote")

target_include_directories(
        libdroid_zygote
        PRIVATE ${JNI_INCLUDE_DIRS}
        PRIVATE ${CMAKE_SOURCE_DIR}/system/core/include
        PRIVATE ${CMAKE_SOURCE_DIR}/frameworks/droid/core/include
)

target_link_libraries(
        libdroid_zygote
        ${JNI_LIBRARIES}
)

if (EXISTS "${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so")
    message(STATUS "Project: when cmake build, there exists /system/core/liblog/liblog.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so "")
endif ()

if (EXISTS "${LIBRARY_OUTPUT_PATH}/frameworks/droid/core/zygote/zygotesocket/libcore_zygotesocket.so")
    message(STATUS "Project: when cmake build
    , there exists /frameworks/droid/core/zygote/zygotesocket/libcore_zygotesocket.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/frameworks/droid/core/zygote/zygotesocket/libcore_zygotesocket.so "")
endif ()


find_library(liblog liblog.so ${LIBRARY_OUTPUT_PATH}/system/core/liblog)
target_link_libraries(libdroid_zygote ${libutils} ${liblog})
find_library(libcore_zygotesocket libcore_zygotesocket.so
        ${LIBRARY_OUTPUT_PATH}/frameworks/droid/core/zygote/zygotesocket/)
target_link_libraries(libdroid_zygote ${libcore_zygotesocket})

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}${relativePath}")