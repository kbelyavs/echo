cmake_minimum_required(VERSION 3.02)
project(server)

set(CMAKE_CXX_STANDARD 11)

add_executable(server_v1 server_v1.cpp)
add_executable(client client.c)

enable_testing()
add_test(test test_v1.sh)