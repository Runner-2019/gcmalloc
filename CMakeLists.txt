cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 17)
project(gcmalloc)
add_executable(
        gcmalloc
        main.cpp
        Chunk.h
        Chunk.cpp
        Bin.h
        Bin.cpp
        Heap.h
        Heap.cpp
        Arena.h
        Arena.cpp
        config.h
        concurrency.h
        gcmalloc.h
        gcmalloc.cpp
        )