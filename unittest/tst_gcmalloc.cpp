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
#define GCMALLOC_DEBUG
#define private public
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>
#include "../include/gcmalloc.h"

using namespace std;

class TST_GCMALLOC : public ::testing::Test {
};

TEST(TST_GCMALLOC, tst_main_arena) {
    gcmalloc gcm;
    mArenaPtr ap = gcm.main_arena();

    /* main_arena should works well with mutex */
    ap->lock_this_arena();
    ASSERT_FALSE(ap->trylock_this_arena());
    ap->unlock_this_arena();
    ASSERT_TRUE(ap->trylock_this_arena());


}


TEST(TST_GCMALLOC, tst_create_arena) {
    gcmalloc gcm;

    /* test minimum size*/
    mArenaPtr ap = gcm.createArena(10);

    /* test new arena's mutex*/
    ap->lock_this_arena();
    ASSERT_FALSE(ap->trylock_this_arena());
    ap->unlock_this_arena();
    ASSERT_TRUE(ap->trylock_this_arena());
    ap->unlock_this_arena();

}


TEST(TST_GCMALLOC, tst_get_arena) {
    gcmalloc gcm;
    /* test for main_arena */
    EXPECT_TRUE(gcm.main_arena()->contiguous());
    EXPECT_FALSE(gcm.main_arena()->have_fast_chunks());

    /* should get main_arena */
    mArenaPtr ap = gcm.get_arena(1);
    ASSERT_EQ(gcm.main_arena(), ap);

    /* Now we have locked the main_arena, let gcmalloc to create new one */
    mArenaPtr ap_new = gcm.get_arena(1);
    EXPECT_TRUE(ap != ap_new);

    /* Now we have locked the ap_new, let gcmalloc to create new new one */
    mArenaPtr  ap_new_new = gcm.get_arena(1);
    EXPECT_TRUE(ap_new_new != ap_new);

    /* Now we unlock main_arena's lock, let gcmalloc using it*/
    gcm.main_arena()->unlock_this_arena();
    mArenaPtr  ap_nn_new = gcm.get_arena(1);
    EXPECT_EQ(ap_nn_new, gcm.main_arena());

    /*
     * Now we unlock ap_new's lock,
     * but gcmalloc don't use it since main_arena is in tsd and it is unlocked
    */
    gcm.main_arena()->unlock_this_arena();
    ap_new->unlock_this_arena();
    mArenaPtr  ap_nnn_new = gcm.get_arena(1);
    EXPECT_EQ(ap_nnn_new, gcm.main_arena());
}

TEST(TST_GCMALLOC, tst_user_size_ops){
    gcmalloc gcm;

    /* test min chunksize */
    EXPECT_EQ(gcm.req2size(15),64);

    /* test alignment */
    EXPECT_EQ(gcm.req2size(65),112);

    /* test too big req */
    EXPECT_EQ(gcm.checked_req2size((size_t)(-2 * SIZE_SZ) + 1),0);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}