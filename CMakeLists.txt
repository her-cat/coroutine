cmake_minimum_required(VERSION 3.16)
project(coroutine C)

set(CMAKE_C_STANDARD 99)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")

set(coroutine_includes include)
set(coroutine_sources src/coroutine.c src/scheduler.c src/main.c)

add_executable(coroutine ${coroutine_sources})
target_include_directories(coroutine PRIVATE ${coroutine_includes})
