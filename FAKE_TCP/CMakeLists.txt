# Set the minimum version of CMake required.
cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)

# Set the project name and version.
project(protocol VERSION 1.0 LANGUAGES CXX)

# C++ standard settings.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE DEBUG)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message("In-source builds are not allowed.")
endif ()

file(GLOB_RECURSE SRC_FILES RELATIVE ${CMAKE_SOURCE_DIR} 
    "src/**.cc"
    "src/*.c"
)

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE include)

enable_testing()
add_test(
    test
    ./${PROJECT_NAME} 
)