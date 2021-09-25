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
#include <iostream>
#include <csignal>
#include <unistd.h>
#include "../include/Heap.h"

using namespace std;
using namespace testing;
class TST_HEAP : testing::Test {
};

class Environment : public testing::Environment {
public:
    void SetUp() override {}
    void TearDown() override {}
};


TEST(TST_HEAP, tst_new_heap) {

    // 1. test min heap size
    auto hp = new_heap(1, 1);
    EXPECT_EQ(hp->get_size(), HEAP_MIN_SIZE);
    hp->~Heap(); // don't use operator 'delete'

    // 2. test max heap size
    hp = new_heap(1024 * 1024 + 1, 0);
    EXPECT_EQ(nullptr, hp);
    hp->~Heap(); // don't use operator 'delete'

    // 3. test ordinary heap size
    hp = new_heap(33 * 1024, 0);
    ASSERT_EQ(hp->get_size(), (33 * 1024 + 4095) & ~4095);
}

TEST(TST_HEAP, tst_grow) {
    // grow too much
    auto hp = new_heap(64 * 4096, 0);

    EXPECT_EQ(hp->get_size(), 64 * 4096);
    EXPECT_EQ(hp->grow(HEAP_MAX_SIZE), -1);

    // normalize action
    hp->grow(DEFAULT_PAGESIZE);
    EXPECT_EQ(hp->get_size(), 65 * 4096);
}

TEST(TST_HEAP, tst_shrink) {
    signal(SIGBUS, [](int n) {
        cout << sys_siglist[n] << endl;
        throw bad_alloc{};
    }); // signal to detect bad alloc

    auto hp = new_heap(64 * 4096, 0);
    EXPECT_EQ(hp->get_size(), 64 * 4096);

    // normalize action
    hp->shrink(4096);
    EXPECT_EQ(hp->get_size(), 63 * 4096);
    *((char *) hp + 4096) = 1; // safe visit
    // visited shrink-ed memory, must throw a bad_alloc
    // 不太好检测
}

TEST(TST_HEAP, tst_dtor) {
    signal(SIGSEGV, [](int n) { throw bad_alloc{}; }); // signal to detect bad alloc
    auto hp = new_heap(64 * 4096, 0);
    hp->~Heap();

    /* We have delete hp, so any ops to hp will cause SIGSEGV */
    ASSERT_THROW(hp->set_size(1), bad_alloc);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

