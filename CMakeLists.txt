cmake_minimum_required(VERSION 3.27)
project(tsc_benchmark VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
endif()

# Library target (header-only)
add_library(tsc_benchmark INTERFACE)
target_include_directories(tsc_benchmark INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(tsc_benchmark INTERFACE cxx_std_20)

# Example executables
add_executable(benchmark_example example.cpp)
target_link_libraries(benchmark_example PRIVATE tsc_benchmark)

# Advanced example (optional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/advanced_example.cpp")
    add_executable(advanced_example examples/advanced_example.cpp)
    target_link_libraries(advanced_example PRIVATE tsc_benchmark)
endif()

# Add pthread for affinity support
find_package(Threads REQUIRED)
target_link_libraries(tsc_benchmark INTERFACE Threads::Threads)

# Enable warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(benchmark_example PRIVATE -Wall -Wextra -Wpedantic)
endif()
