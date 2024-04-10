project(binder-driver)

set(CMAKE_C_FLAGS "-std=gnu90")

add_definitions(
        -D__KERNEL__
        -D__WSL__
        -DKBUILD_MODNAME="binder"
)

include_directories("${CMAKE_BUILD_LINUX_KERNEL}/include/")
include_directories("${CMAKE_BUILD_LINUX_KERNEL}/arch/x86/include/")
include_directories("${CMAKE_BUILD_LINUX_KERNEL}/arch/x86/include/generated/")

if (CLION_IDE STREQUAL "TRUE")
    file(GLOB_RECURSE sourceFiles binder.c binder_alloc.c binderfs.c)
else()
    file(WRITE binder_tmp.c "")
    file(GLOB_RECURSE sourceFiles *_tmp.c)
endif ()

add_library(dbinder_shared SHARED ${sourceFiles})

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}${relativePath}")