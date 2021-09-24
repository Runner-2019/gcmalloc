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
aux_source_directory(unittest TEST_SRC_LIST)
add_executable(UnitTests ${TEST_SRC_LIST} Chunk.cpp)
target_link_libraries(UnitTests gtest)
include(GoogleTest)
#gtest_discover_tests(MyTests)
add_test(Test UnitTests)

