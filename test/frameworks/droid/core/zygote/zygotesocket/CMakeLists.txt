project(zygotesocket_test)

add_executable(ZygoteSocketClientTest ZygoteSocketClientTest.cpp)
add_executable(ZygoteSocketServerTest ZygoteSocketServerTest.cpp)

target_include_directories(
        ZygoteSocketServerTest
        PRIVATE ${CMAKE_SOURCE_DIR}/system/core/include
        PRIVATE ${CMAKE_SOURCE_DIR}/frameworks/droid/core/include
)
target_include_directories(
        ZygoteSocketClientTest
        PRIVATE ${CMAKE_SOURCE_DIR}/system/core/include
        PRIVATE ${CMAKE_SOURCE_DIR}/frameworks/droid/core/include
)

if (EXISTS "${LIBRARY_OUTPUT_PATH}/frameworks/droid/core/zygote/zygotesocket/libcore_zygotesocket.so")
    message(STATUS "Project: when cmake build
    , there exists /frameworks/droid/core/zygote/zygotesocket/libcore_zygotesocket.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/frameworks/droid/core/zygote/zygotesocket/libcore_zygotesocket.so "")
endif ()

if (EXISTS "${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so")
    message(STATUS "Project: when cmake build, there exists /system/core/liblog/liblog.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so "")
endif ()

find_library(liblog liblog.so ${LIBRARY_OUTPUT_PATH}/system/core/liblog)
target_link_libraries(ZygoteSocketClientTest ${liblog})
target_link_libraries(ZygoteSocketServerTest ${liblog})

find_library(libcore_zygotesocket libcore_zygotesocket.so
        ${LIBRARY_OUTPUT_PATH}/frameworks/droid/core/zygote/zygotesocket/)
target_link_libraries(ZygoteSocketClientTest ${libcore_zygotesocket})
target_link_libraries(ZygoteSocketServerTest ${libcore_zygotesocket})

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH  ${EXECUTABLE_OUTPUT_PATH}${relativePath})
