project(libbinder)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_library(libbinder SHARED
        Static.cpp
        Parcel.cpp
        Status.cpp
        Binder.cpp
        BpBinder.cpp
        IPCThreadState.cpp
        IServiceManager.cpp
        ProcessState.cpp)
set_target_properties(libbinder PROPERTIES OUTPUT_NAME "binder")

target_include_directories(
        libbinder
        PRIVATE ${PROJECT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_SOURCE_DIR}/system/core/include
        PRIVATE ${CMAKE_SOURCE_DIR}/frameworks/idl/include
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

if (EXISTS "${LIBRARY_OUTPUT_PATH}/frameworks/idl/native/libs/binder/libidlbinder.so")
    message(STATUS "Project: when cmake build, there exists /frameworks/idl/native/libs/binder/libidlbinder.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/frameworks/idl/native/libs/binder/libidlbinder.so "")
endif ()

find_library(libutils libutils.so ${LIBRARY_OUTPUT_PATH}/system/core/libutils)
target_link_libraries(libbinder ${libutils})

find_library(liblog liblog.so ${LIBRARY_OUTPUT_PATH}/system/core/liblog)
target_link_libraries(libbinder ${liblog})

find_library(libidlbinder libidlbinder.so ${LIBRARY_OUTPUT_PATH}/frameworks/idl/native/libs/binder)
target_link_libraries(libbinder ${libidlbinder})

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}${relativePath}")

