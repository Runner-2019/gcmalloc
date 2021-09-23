# CMake generated Testfile for 
# Source directory: /Users/zhangnaigang/CLionProjects/gcmalloc
# Build directory: /Users/zhangnaigang/CLionProjects/gcmalloc/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/Users/zhangnaigang/CLionProjects/gcmalloc/build/MyTests[1]_include.cmake")
add_test([=[Test]=] "MyTests")
set_tests_properties([=[Test]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/zhangnaigang/CLionProjects/gcmalloc/CMakeLists.txt;16;add_test;/Users/zhangnaigang/CLionProjects/gcmalloc/CMakeLists.txt;0;")
subdirs("_deps/googletest-build")
