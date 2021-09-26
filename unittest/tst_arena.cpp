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
#include "../include/Arena.h"
using namespace std;

class TST_ARENA : public ::testing::Test {
};

TEST(TST_ARENA, tst_arena_fast_chunk_bit) {
    Arena a;
    EXPECT_TRUE(a.have_fast_chunks());
    a.set_fast_chunks();
    EXPECT_TRUE(a.have_fast_chunks());
    a.clear_fast_chunks();
    EXPECT_FALSE(a.have_fast_chunks());
    a.set_fast_chunks();
    EXPECT_TRUE(a.have_fast_chunks());
}


TEST(TST_ARENA, tst_arena_contiguous_bit) {
    Arena a;
    EXPECT_TRUE(a.contiguous());
    a.set_non_contiguous();
    EXPECT_TRUE(a.non_contiguous());
    a.set_contiguous();
    EXPECT_TRUE(a.contiguous());
}

TEST(TST_ARENA, tst_unsorted_chunks) {
    Arena a;
    /* no need to test now */
}

TEST(TST_ARENA, tst_init) {
    Arena a, b;

    // main arena
    a.init(THIS_IS_MAIN_ARENA);
    EXPECT_FALSE(a.have_fast_chunks());
    EXPECT_TRUE(a.contiguous());
    EXPECT_FALSE(a.non_contiguous());

    // non main arena
    b.init(THIS_IS_NON_MAIN_ARENA);
    EXPECT_FALSE(b.have_fast_chunks());
    EXPECT_FALSE(b.contiguous());
    EXPECT_TRUE(b.non_contiguous());

}

TEST(TST_ARENA, tst_lock) {
    Arena arena;
    arena.lock_this_arena();
    EXPECT_FALSE(arena.trylock_this_arena());
    arena.unlock_this_arena();
    EXPECT_TRUE(arena.trylock_this_arena());
    arena.unlock_this_arena();

    Arena a1, a2;
    a1.lock_this_arena();
    a2.lock_this_arena();
    EXPECT_FALSE(a1.trylock_this_arena());
    EXPECT_FALSE(a2.trylock_this_arena());
    a1.unlock_this_arena();
    a2.unlock_this_arena();

}

TEST(TST_ARENA, tst_heap_trim) {

}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}