cmake_minimum_required(VERSION 3.10)

project(redis)

# Compiler Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -fpermissive") 
add_compile_options(-fPIC)  

# Compiliing as shared lib
add_library(server SHARED server.cpp)
add_library(client SHARED client.cpp)

# Executables
add_executable(main_server main_server.cpp)
add_executable(main_client main_client.cpp)

# Linking
target_link_libraries(main_server server)
target_link_libraries(main_client client)
