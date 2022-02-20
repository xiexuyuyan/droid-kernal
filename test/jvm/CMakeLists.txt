project(CreateJvm)

set(BUILD_USE_64BITS ON)

find_package(JNI REQUIRED)

add_executable(CreateJvm CreateJvm.cpp)

target_include_directories(
        CreateJvm
        PRIVATE ${JNI_INCLUDE_DIRS}
)

target_link_libraries(
        CreateJvm
        ${JAVA_JVM_LIBRARY}
)

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH  ${EXECUTABLE_OUTPUT_PATH}${relativePath})