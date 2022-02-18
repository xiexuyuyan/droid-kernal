project(untitled)

cmake_minimum_required(VERSION 3.16)

set(LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/out/lib)
set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/out/bin)

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/out/_install)

#set(CMAKE_CXX_COMPILER  "clang++" )
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS "-g" )
set(CMAKE_CXX_FLAGS "-Wall" )

# 调试包不优化
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
# release包优化
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# add_subdirectory(system/core/libutils)
# add_subdirectory(system/core/liblog)
add_subdirectory(system/core)
add_subdirectory(frameworks/av/media/mediaserver)

add_subdirectory(test/system/core)

# add_subdirectory(common/drivers/staging/log)
add_subdirectory(test/common/drivers/staging/log)

add_subdirectory(frameworks/droid/jni)