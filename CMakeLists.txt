cmake_minimum_required(VERSION 3.29)
project(chip8_redo C)

set(CMAKE_C_STANDARD 11)

add_executable(chip8_redo chip8.c
        src/config.h
        src/colorp.h
)
