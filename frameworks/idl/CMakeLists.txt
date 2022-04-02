project(frameworks-idl)

# add include directories to build
include_directories(AFTER "include")
include_directories(${CMAKE_SOURCE_DIR}/frameworks/native/include)

add_subdirectory(native/libs/binder)