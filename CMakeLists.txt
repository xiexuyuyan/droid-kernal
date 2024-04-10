project(untitled)

cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED CLION_IDE)
    set(CLION_IDE $ENV{CLION_IDE})
endif()

if(NOT DEFINED CMAKE_BUILD_LINUX_KERNEL)
    set(CMAKE_BUILD_LINUX_KERNEL $ENV{CMAKE_BUILD_LINUX_KERNEL})
endif()

set(LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/out/lib)
set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/out/bin)

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/out/_install)

#set(CMAKE_CXX_COMPILER  "/usr/bin/clang++" )
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS "-g" )
set(CMAKE_CXX_FLAGS "-Wall" )

# 调试包不优化
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
# release包优化
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

add_subdirectory(common/drivers/staging/binder)
add_subdirectory(common/drivers/staging/log)

add_subdirectory(system/core)

add_subdirectory(frameworks/droid/runtime/jni)
add_subdirectory(frameworks/droid/core/zygote/zygotesocket)
add_subdirectory(frameworks/droid/core/zygote/jni)

add_subdirectory(frameworks/av/media/mediaserver)

add_subdirectory(frameworks/native)

add_subdirectory(frameworks/idl)



add_subdirectory(test/common/drivers/staging/log)

# add_subdirectory(test/common/clib)

add_subdirectory(test/system/core)

add_subdirectory(test/frameworks/droid/core/zygote/zygotesocket)

add_subdirectory(test/jvm)

add_subdirectory(test/DroidThread)
