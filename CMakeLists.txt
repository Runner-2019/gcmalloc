cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 17)
project(gcmalloc)
add_executable(gcmalloc
        main.cpp
        arena.h
        arena.cpp
        config.h
        concurrency.h
        gcmalloc.h
        gcmalloc.cpp
        Chunk.h
        Chunk.cpp
        )