add_library(server SHARED server.cpp)
add_library(client SHARED client.cpp)
add_library(parser SHARED parser.cpp)

# Executables
add_executable(main_server main_server.cpp)
add_executable(main_client main_client.cpp)


# Linking
target_link_libraries(main_server server parser) 
target_link_libraries(main_client client parser) 
