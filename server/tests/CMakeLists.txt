cmake_minimum_required(VERSION 3.10.2)

project(Instant_Messenger_for_Linux_server_test)

set(CMAKE_CXX_FLAGS " -std=c++14 -lgtest_main -lgtest -pthread")

add_executable(testing tests.cpp)
