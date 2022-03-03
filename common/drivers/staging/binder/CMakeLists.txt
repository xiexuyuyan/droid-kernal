project(binder-driver)

set(CMAKE_C_FLAGS "-std=gnu90")

add_definitions(
        -D__KERNEL__
)

include_directories("/root/linux/linux-5.10.16.3-wsl/include/")
include_directories("/root/linux/linux-5.10.16.3-wsl/arch/x86/include/")
include_directories("/root/linux/linux-5.10.16.3-wsl/arch/x86/include/generated/")

add_library(dbinder_shared SHARED binder.c)