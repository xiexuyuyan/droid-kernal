project(log-driver)

set(CMAKE_C_FLAGS "-std=gnu99")

include_directories("/root/linux/linux-5.10.16.3-wsl/include/")
# include_directories("/root/linux/linux-4.4.302/include/")

file(GLOB_RECURSE sourceFiles ./*.c)

add_library(dlog_shared SHARED ${sourceFiles})
# target_compile_options(dlog_shared PRIVATE "--std=gnu99")
# add_compile_options(CMAKE_C_FLAGS "-std=gnu99")
