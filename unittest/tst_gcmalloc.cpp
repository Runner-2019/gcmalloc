/*
  --------------------------------------
     ╭╩══╮╔══════╗╔══════╗╔══════╗ 
    ╭    ╠╣      ╠╣      ╠╣      ╣
    ╰⊙══⊙╯╚◎════◎╝╚◎════◎╝╚◎════◎╝
  --------------------------------------
  @date:   2021-九月-24
  @author: xiaomingZhang2020@outlook.com
  --------------------------------------
*/

#include <gtest/gtest.h>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>
#include "../include/gcmalloc.h"
using namespace std;

class TST_GCMALLOC : public ::testing::Test {
};


TEST(TST_GCMALLOC, tst_demo){

}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}