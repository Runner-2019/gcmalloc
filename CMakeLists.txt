cmake_minimum_required (VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
project(gcmalloc)


# main
aux_source_directory(. SRC_LIST)
add_executable(gcmalloc ${SRC_LIST})

# gtest
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/40dfd4b775a66979ad1bd19321cdfd0feadfea27.zip
)
FetchContent_MakeAvailable(googletest)
enable_testing()
include(GoogleTest)
aux_source_directory(TST_CHUNK TEST_SRC_LIST)

# tst_chunk
add_executable(test_chunk unittest/tst_chunk.cpp Chunk.cpp)
target_link_libraries(test_chunk gtest)
add_test(Test test_chunk)

# gtest: tst_heap
add_executable(test_heap unittest/tst_heap.cpp Heap.cpp)
target_link_libraries(test_heap gtest)
add_test(Test test_heap)


