cmake_minimum_required(VERSION 3.02)
project(server)

set(CMAKE_CXX_STANDARD 11)

add_executable(server_v1 server_v1.cpp)
add_executable(server_v2 server_v2.cpp)
add_executable(server_v3 server_v3.cpp)
add_executable(client client.c)

enable_testing()
add_test(test test_v1.sh)
add_test(test test_v2.sh)