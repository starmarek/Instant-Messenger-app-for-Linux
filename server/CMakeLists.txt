cmake_minimum_required(VERSION 3.10.2)
project(Instant_Messenger_for_Linux_server)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -pedantic")

include_directories(bin)

file(GLOB SOURCES "src/*.cpp")

add_executable(server ${SOURCES})
    
