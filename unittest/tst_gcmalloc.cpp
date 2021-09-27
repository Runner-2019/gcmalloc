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
    mArenaPtr ap_new_new = gcm.get_arena(1);
    EXPECT_TRUE(ap_new_new != ap_new);

    /* Now we unlock main_arena's lock, let gcmalloc using it*/
    gcm.main_arena()->unlock_this_arena();
    mArenaPtr ap_nn_new = gcm.get_arena(1);
    EXPECT_EQ(ap_nn_new, gcm.main_arena());

    /*
     * Now we unlock ap_new's lock,
     * but gcmalloc don't use it since main_arena is in tsd and it is unlocked
    */
    gcm.main_arena()->unlock_this_arena();
    ap_new->unlock_this_arena();
    mArenaPtr ap_nnn_new = gcm.get_arena(1);
    EXPECT_EQ(ap_nnn_new, gcm.main_arena());
}

TEST(TST_GCMALLOC, tst_user_size_ops) {
    gcmalloc gcm;

    /* test min chunksize */
    EXPECT_EQ(gcm.req2size(15), 64);

    /* test alignment */
    EXPECT_EQ(gcm.req2size(65), 112);

    /* test too big req */
    EXPECT_EQ(gcm.checked_req2size((size_t) (-2 * SIZE_SZ) + 1), 0);
}

TEST(TST_GCMALLOC, tst_check_chunk) {
    /* now we only guarantee check__xxx could run */
    gcmalloc gcm;
    mArenaPtr  ap = gcm.createArena(1);
    gcm.check_chunk(ap,ap->top_chunk);
    gcm.check_malloc_state(ap);


}

TEST(TST_GCMALLOC, tst_malloc_from_sys) {
    gcmalloc gcm;
    /* test allocated large size using mmap */
    auto memp = (char*)gcm.malloc_from_sys(200*1024*1024, gcm.main_arena());
    auto cp = (mChunkPtr) (memp - 2 * SIZE_SZ);
    EXPECT_TRUE(cp->is_mmapped());
    EXPECT_EQ(cp->get_size(), 200 * 1024 * 1024);

    /* test non_main_arena use new heap*/
    // we lock this arena, force the gcm to create new one
    gcm.main_arena()->lock_this_arena();
    auto non_main_arena = gcm.get_arena(1);
    ASSERT_TRUE(gcm.main_arena() != non_main_arena);
    auto old_top = non_main_arena->top_chunk;

    /* simulate old chunk's size is used out */
    const int new_top_size = 4096;
    const int old_top_size = old_top->get_size();
    auto new_top = (mChunkPtr)((char*)old_top + old_top->get_size() - new_top_size);
    new_top->set_head_size(new_top_size);
    new_top->set_prev_size(old_top->get_size() - new_top_size);
    old_top->set_head( old_top->get_size() - new_top_size | PREV_INUSE);
    old_top->set_inuse();
    non_main_arena->top_chunk = new_top;

    auto use_old_heap = (char*)gcm.malloc_from_sys(4096 * 2, non_main_arena);
    auto use_old_heap_chunk = (mChunkPtr)(use_old_heap - 2 * SIZE_SZ);
    EXPECT_EQ(use_old_heap_chunk->get_size(),4096 * 2);
//    EXPECT_EQ(use_old_heap_chunk - 4096 * 2 ,old_top); /* used old heap*/

    /* test non_main_arena create a new heap*/


    /* test the fencepost of new heap*/

    /* test the foreign sbrk call */

    /* */


}


TEST(TST_GCMALLOC, tst_free){
    /* tst free nullptr */
    free(nullptr);



}

TEST(TST_GCMALLOC, tst_int_malloc){
    gcmalloc gcm;
    auto main_arena_ptr = gcm.main_arena();
    /* test allocated from fast bin */
    auto mem = gcm._int_malloc(main_arena_ptr,20);
    auto mem_cp = offset2Chunk(mem, 2 * SIZE_SZ);
//    EXPECT_TRUE(mem_cp->prev_inuse());

    /* test allocated from small bin */



    /* test allocated from large bin */


    /*  */
    /*  */

}

TEST(TST_GCMALLOC, tst_malloc){
    gcmalloc gcm;
    gcm.malloc(20);
    gcm.malloc(200);
    gcm.malloc(2000);

}

TEST(TST_GCMALLOC, tst_malloc_and_free){
    gcmalloc gc;
    auto p1 = gc.malloc(20);
    gc.free(p1);

    auto p2 = gc.malloc(200);
    gc.free(p2);

    auto p3 = gc.malloc(2000);
    gc.free(p3);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}