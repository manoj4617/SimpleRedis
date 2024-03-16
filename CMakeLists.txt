cmake_minimum_required(VERSION 3.10)
project(redis)

# Compiler Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -fpermissive") 
add_compile_options(-fPIC)  

# Locate GoogleTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Process subdirectories for source and tests
add_subdirectory(src)
add_subdirectory(test)