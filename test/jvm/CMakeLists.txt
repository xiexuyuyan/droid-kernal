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