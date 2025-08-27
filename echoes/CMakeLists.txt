cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(echoes)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

