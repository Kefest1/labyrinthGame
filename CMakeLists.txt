cmake_minimum_required(VERSION 3.23)
project(labyrinthGame C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(labyrinthGame server.c player.h server.h utils.h serverInformationStatistics.h)

add_executable(labyrinthGame_2 player.h server.h utils.h player.c)
