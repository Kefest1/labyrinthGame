cmake_minimum_required(VERSION 3.23)
project(labyrinthGame C)

set(CMAKE_C_STANDARD 99)

add_executable(labyrinthGame server.c player.h server.h utils.h)  # player.c wild_beast.c)
