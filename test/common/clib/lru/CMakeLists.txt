project(LruExample)

set(CMAKE_C_FLAGS "-std=gnu90")

add_definitions(
        -D__KERNEL__
        -D__WSL__
        -DKBUILD_MODNAME="lru-example"
)

include_directories("/home/yuyan/projects/linux/linux-kernel-wsl/include/")
include_directories("/home/yuyan/projects/linux/linux-kernel-wsl/arch/x86/include/")
include_directories("/home/yuyan/projects/linux/linux-kernel-wsl/arch/x86/include/generated/")

file(GLOB_RECURSE sourceFiles LruMain.c LruShrinker.c)
add_library(LruExample_shared SHARED ${sourceFiles})
