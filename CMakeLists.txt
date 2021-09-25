cmake_minimum_required (VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
project(gcmalloc)


# main
include_directories(include)
aux_source_directory(. SRC_LIST)
add_executable(gcmalloc ${SRC_LIST} )

# gtest
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/40dfd4b775a66979ad1bd19321cdfd0feadfea27.zip
)
FetchContent_MakeAvailable(googletest)
enable_testing()
include(GoogleTest)

# tst_chunk
add_executable(test_chunk unittest/tst_chunk.cpp Chunk.cpp)
target_link_libraries(test_chunk gtest)
add_test(Test test_chunk)

# gtest: tst_heap
add_executable(test_heap unittest/tst_heap.cpp Heap.cpp)
target_link_libraries(test_heap gtest)
add_test(Test test_heap)

# gtest: tst_bin
add_executable(test_bin unittest/tst_bin.cpp Bin.cpp Chunk.cpp)
target_link_libraries(test_bin gtest)
add_test(Test test_bin)

# gtest: tst_arena
add_executable(test_arena unittest/tst_arena.cpp Arena.cpp Bin.cpp Chunk.cpp Heap.cpp)
target_link_libraries(test_arena gtest)
add_test(Test test_arena)

# gtest: tst_concurrency
add_executable(test_concurrency include/concurrency.h unittest/tst_concurrency.cpp)
target_link_libraries(test_concurrency gtest)
add_test(Test test_concurrency)

# gtest: tst_gcmalloc
add_executable(test_gcmalloc unittest/tst_gcmalloc.cpp ${SRC_LIST})
target_link_libraries(test_gcmalloc gtest)
add_test(Test test_gcmalloc)