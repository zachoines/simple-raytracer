cmake_minimum_required(VERSION 3.13)  # CMake version check
project(SimpleRayTracer)
set(CMAKE_CXX_STANDARD 20)            # Enable c++20 standard

# Add main.cpp file of project root directory as source file
file(GLOB_RECURSE SOURCES "main.cpp" "/src/*.h")

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(SimpleRayTracer ${SOURCES})