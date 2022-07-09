cmake_minimum_required(VERSION 3.10)

project(udp-chat)

add_executable(chat_client src/udp_client.c)
add_executable(chat_server src/udp_server.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wfatal-errors -Wall -pthread")