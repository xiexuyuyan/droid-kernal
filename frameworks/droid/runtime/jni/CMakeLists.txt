project(LibDroidRuntime)

# /mnt/c/Program Files/Java/jdk1.8.0_231/include/jni.h
# /usr/lib/jvm/java-8-openjdk-amd64/include/
# /mnt/c/Users/23517/AppData/Local/JetBrains/Toolbox/apps/IDEA-U/ch-0/213.6461.79/jbr/include
set(JNI_INCLUDE_PATH "/usr/lib/jvm/java-8-openjdk-amd64/include")

find_package(JNI REQUIRED)

file(GLOB_RECURSE sourceFiles ./*.cpp)

add_library(libdroid_runtime_shared SHARED ${sourceFiles})

# set build/lib 's output name
set_target_properties(libdroid_runtime_shared PROPERTIES OUTPUT_NAME "droid_runtime")
# set_target_properties(libdroid_runtime_shared PROPERTIES VERSION 1.0 SOVERSION 1)

# include_directories("${JNI_INCLUDE_PATH}")
target_include_directories(
        libdroid_runtime_shared
        PRIVATE ${JNI_INCLUDE_DIRS}
        PRIVATE ${CMAKE_SOURCE_DIR}/system/core/include
)

target_link_libraries(
        libdroid_runtime_shared
        ${JNI_LIBRARIES}
)

if (EXISTS "${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so")
    message(STATUS "Project: when cmake build, there exists /system/core/liblog/liblog.so")
else()
    file(WRITE ${LIBRARY_OUTPUT_PATH}/system/core/liblog/liblog.so "")
endif ()

find_library(liblog liblog.so ${LIBRARY_OUTPUT_PATH}/system/core/liblog)
target_link_libraries(libdroid_runtime_shared ${libutils} ${liblog})



string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
# build path
set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}${relativePath}")