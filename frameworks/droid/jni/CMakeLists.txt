project(LibDroidRuntime)

# /mnt/c/Program Files/Java/jdk1.8.0_231/include/jni.h
# /usr/lib/jvm/java-8-openjdk-amd64/include/
# /mnt/c/Users/23517/AppData/Local/JetBrains/Toolbox/apps/IDEA-U/ch-0/213.6461.79/jbr/include
set(JNI_INCLUDE_PATH "/usr/lib/jvm/java-8-openjdk-amd64/include")

set(CMAKE_C_FLAGS
        -fPIC
        -I "${JNI_INCLUDE_PATH}/linux"
        -I "${JNI_INCLUDE_PATH}"
        -shared
        )

file(GLOB_RECURSE sourceFiles ./*.cpp)

add_library(libdroid_runtime_shared SHARED ${sourceFiles})

# set build/lib 's output name
set_target_properties(libdroid_runtime_shared PROPERTIES OUTPUT_NAME "droid_runtime")
set_target_properties(libdroid_runtime_shared PROPERTIES VERSION 1.0 SOVERSION 1)

# include_directories("${JNI_INCLUDE_PATH}")
target_include_directories(
        libdroid_runtime_shared
        PRIVATE "${JNI_INCLUDE_PATH}"
        PRIVATE "${JNI_INCLUDE_PATH}/linux")

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
# build path
set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}${relativePath}")