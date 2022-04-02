project(servicemanager)

add_executable(ServiceManager main.cpp ServiceManager.cpp ServiceManager.h)

TARGET_INCLUDE_DIRECTORIES(
        ServiceManager
        PRIVATE ${CMAKE_SOURCE_DIR}/frameworks/native/include
        PRIVATE ${CMAKE_SOURCE_DIR}/system/core/include
        PRIVATE ${CMAKE_SOURCE_DIR}/frameworks/idl/include
)

if (EXISTS "${LIBRARY_OUTPUT_PATH}/system/core/libutils/libutils.so")
    message(STATUS "Project: when cmake build, there exists /system/core/libutils/libutils.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/system/core/libutils/libutils.so "")
endif ()

if (EXISTS "${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so")
    message(STATUS "Project: when cmake build, there exists /system/core/liblog/liblog.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so "")
endif ()

if (EXISTS "${LIBRARY_OUTPUT_PATH}/frameworks/native/libs/binder/libbinder.so")
    message(STATUS "Project: when cmake build, there exists /frameworks/native/libs/binder/libbinder.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/frameworks/native/libs/binder/libbinder.so "")
endif ()

if (EXISTS "${LIBRARY_OUTPUT_PATH}/frameworks/idl/native/libs/binder/libidlbinder.so")
    message(STATUS "Project: when cmake build, there exists /frameworks/idl/native/libs/binder/libidlbinder.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/frameworks/idl/native/libs/binder/libidlbinder.so "")
endif ()

FIND_LIBRARY(libutils libutils.so ${LIBRARY_OUTPUT_PATH}/system/core/libutils)
FIND_LIBRARY(liblog liblog.so ${LIBRARY_OUTPUT_PATH}/system/core/liblog)
FIND_LIBRARY(libbinder libbinder.so ${LIBRARY_OUTPUT_PATH}/frameworks/native/libs/binder)
FIND_LIBRARY(libidlbinder libidlbinder.so ${LIBRARY_OUTPUT_PATH}/frameworks/idl/native/libs/binder)
TARGET_LINK_LIBRARIES(ServiceManager ${libutils} ${liblog} ${libidlbinder} ${libbinder})

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH  ${EXECUTABLE_OUTPUT_PATH}${relativePath})