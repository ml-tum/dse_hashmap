cmake_minimum_required(VERSION 3.24)
project(dse_hashmap)

set(CMAKE_CXX_STANDARD 17)

add_executable(stress_test my_hashmap.hpp stress_test.cpp)

add_executable(server my_hashmap.hpp server.cpp shared_mem.h)

add_executable(client shared_mem.h client.cpp)
