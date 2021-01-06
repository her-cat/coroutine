cmake_minimum_required(VERSION 3.16)
project(coroutine C)

set(CMAKE_C_STANDARD 99)

set(coroutine_includes include)
set(coroutine_sources src/coroutine.c)

add_executable(coroutine ${coroutine_sources})
target_include_directories(coroutine PRIVATE ${coroutine_includes})
