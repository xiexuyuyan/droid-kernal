project(binder-driver)

set(CMAKE_C_FLAGS "-std=gnu90")

add_definitions(
        -D__KERNEL__
        -DKBUILD_MODNAME="binder"
)

include_directories("/root/linux/linux-kernel-wsl/include/")
include_directories("/root/linux/linux-kernel-wsl/arch/x86/include/")
include_directories("/root/linux/linux-kernel-wsl/arch/x86/include/generated/")

if (DEFINED CMAKE_BUILD_TYPE)
    if (CMAKE_BUILD_TYPE STREQUAL "Clion")
        file(GLOB_RECURSE sourceFiles binder.c binder_alloc.c binderfs.c)
    else()
        file(WRITE binder_tmp.c "")
        file(GLOB_RECURSE sourceFiles *_tmp.c)
    endif ()
else()
    file(WRITE binder_tmp.c "")
    file(GLOB_RECURSE sourceFiles *_tmp.c)
endif ()

add_library(dbinder_shared SHARED ${sourceFiles})

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}${relativePath}")