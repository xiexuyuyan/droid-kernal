project(OpenLogDriver)

add_executable(OpenLogDriver OpenLogDriver.cpp)
add_executable(LogPrinter LogPrinter.cpp)

string(REPLACE ${CMAKE_SOURCE_DIR} "" relativePath ${PROJECT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH  ${EXECUTABLE_OUTPUT_PATH}${relativePath})