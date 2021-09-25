/*
  --------------------------------------
     ╭╩══╮╔══════╗╔══════╗╔══════╗
    ╭    ╠╣      ╠╣      ╠╣      ╣
    ╰⊙══⊙╯╚◎════◎╝╚◎════◎╝╚◎════◎╝
  --------------------------------------
  @date:   2021-九月-23
  @author: xiaomingZhang2020@outlook.com
  --------------------------------------
*/
#include <gtest/gtest.h>
#include "../include/Chunk.h"
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>

using namespace std;
using ::testing::Test;

class TST_CHUNK : public ::testing::Test {};


class Environment : public ::testing::Environment {
public:
    void SetUp() override {
        cout << "Allocated 200 bytes to test chunk ops.\n";
        p = (char *) mmap(nullptr, 200, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
        if (p == MAP_FAILED) {
            cout<<strerror(errno)<<endl;
        }
    }

    void TearDown() override {
        munmap((void *) p, 200);
        cout << "Freed 200 bytes.\n";
    }
public:
    char *p{};
};

Environment *envp = new Environment();

/* test the constant */
TEST(TST_CHUNKS, tst_chunk_constant) {
    /* basic size constant */
    ASSERT_EQ(SIZE_SZ, 8);
    ASSERT_EQ(GCMALLOC_ALIGNMENT, 16);
    ASSERT_EQ(GCMALLOC_ALIGN_MASK, 15);

    /* small bins constant */
    ASSERT_EQ(SMALL_BIN_WIDTH, 16);
    ASSERT_EQ(NUM_SMALL_BINS, 64);
    ASSERT_EQ(SMALL_BINS_MAX_SIZE, 64 * 16);
}

TEST(TST_CHUMKS, tst_in_smallbin_range) {
    EXPECT_TRUE(in_smallbin_range(1));
    EXPECT_TRUE(in_smallbin_range(64 * 16 - 1));
    EXPECT_FALSE(in_smallbin_range(64 * 16));
    EXPECT_FALSE(in_smallbin_range(64 * 16 + 1));
}

/*
 * [1]                 [2]   [3]     [4]    [5]  ......   [63]
 * unsorted chunks:   32-47  48-63   64-79  80-75 ...... 63 * 16 -> 64 * 16 - 1
 */
TEST(TST_CHUNKS, tst_smallbin_idx) {
    EXPECT_EQ(smallbin_idx(32), 2);
    EXPECT_EQ(smallbin_idx(40), 2);
    EXPECT_EQ(smallbin_idx(47), 2);
    EXPECT_EQ(smallbin_idx(48), 3);
    EXPECT_EQ(smallbin_idx(1023), 63);
    EXPECT_EQ(smallbin_idx(1024), 64);
}

TEST(TST_CHUNKS, tst_larginbin_idx) {
    EXPECT_EQ(largebin_idx(1023), 63);
    EXPECT_EQ(largebin_idx(1024), 64);
    EXPECT_EQ(largebin_idx(1024 + 63), 64);
    EXPECT_EQ(largebin_idx(1024 + 64), 65);

    EXPECT_EQ(largebin_idx(3071), 95);
    EXPECT_EQ(largebin_idx(3072), 96);
    EXPECT_EQ(largebin_idx(3073), 96);
    EXPECT_EQ(largebin_idx(3135), 96);
    EXPECT_EQ(largebin_idx(3136), 97);
    EXPECT_EQ(largebin_idx(3137), 97);
    EXPECT_EQ(largebin_idx(3136 + 447), 97);
    EXPECT_EQ(largebin_idx(3136 + 448), 98);
    EXPECT_EQ(largebin_idx(3136 + 448 + 511), 98);

    EXPECT_EQ(largebin_idx(1024), 64);
    EXPECT_EQ(largebin_idx(3071), 95);
    EXPECT_EQ(largebin_idx(3072), 96);
    EXPECT_EQ(largebin_idx(3135), 96);
    EXPECT_EQ(largebin_idx(3135), 96);
    EXPECT_EQ(largebin_idx(3136), 97);
    EXPECT_EQ(largebin_idx(1e10), 126);
}


TEST(TST_CHUNKS, tst_this_chunks) {
    auto cp = (mChunkPtr) (envp->p);
    auto p = cp->this_chunk();
    ASSERT_EQ(p, envp->p);
}


TEST(TST_CHUNKS, tst_size_ops) {
    auto cp = (mChunkPtr) (envp->p);

    cp->set_head(96);
    EXPECT_EQ(cp->get_size(), 96);

    cp->set_head_size(97); // set_head_size won't bother other bits
    EXPECT_TRUE(cp->prev_inuse());

    cp->set_prev_size(96);
    EXPECT_EQ(cp->get_prev_size(), 96);
}


TEST(TST_CHUNKS,tst_nxt_chunk){
    auto cp = (mChunkPtr) (envp->p);
    cp->set_head( 96 | 0);
    auto nxtp = cp->nxt_chunk();
    EXPECT_EQ(nxtp->this_chunk(),cp->this_chunk() + 96);
}


TEST(TST_CHUNKS,tst_pre_chunk){
    auto cp = (mChunkPtr) ((envp->p) + 96);
    cp->set_prev_size( 96 | 0);
    auto prep = cp->pre_chunk();
    EXPECT_EQ(prep->this_chunk(),cp->this_chunk() - 96);
}

TEST(TST_CHUNK, tst_set_footer){
    auto cp = (mChunkPtr)(envp->p);
    cp->set_head_size(96);
    auto nxtp = cp->nxt_chunk();
    cp->set_foot(96);
    ASSERT_EQ(96, nxtp->get_prev_size());
}

TEST(TST_CHUNK, tst_bits_ops){
    auto cp = (mChunkPtr)(envp->p);
    cp->set_head(0x10000 | PREV_INUSE);
    EXPECT_TRUE(cp->prev_inuse());

    cp->set_head(0x10000 | IS_MMAPPED);
    EXPECT_TRUE(cp->is_mmapped());

    cp->set_head(0x10000 | NON_MAIN_ARENA);
    EXPECT_TRUE(cp->is_in_non_main_arena());

    cp->set_head(0x10000 | GC_MARKED);
    EXPECT_TRUE(cp->is_marked());

}

TEST(TST_CHUNK, tst_inuse_ops){
    auto cp = (mChunkPtr)(envp->p);
    cp->set_head_size(96);
    auto nxtp = offset2Chunk(cp,96);
    nxtp->set_prev_size(96);
    nxtp->set_head_size(64);

    cp->set_inuse();
    EXPECT_EQ(nxtp->get_size_field(), 65);
    cp->clear_inuse();
    EXPECT_EQ(nxtp->get_size_field(), 64 );
    EXPECT_TRUE(!cp->inuse());
}

TEST(TST_CHUNK, tst_chunk2mem){
    auto cp = (mChunkPtr)(envp->p);
    ASSERT_EQ(cp->chunk2mem(),envp->p + 16);
}

TEST(TST_CHUNK, tst_belonged_heap){
    auto cp = (mChunkPtr)(envp->p);


}


TEST(TST_CHUNK, tst_get_valid_size){
    auto cp = (mChunkPtr)(envp->p);
    cp->set_head_size(96);
    ASSERT_EQ(cp->get_valid_size(), 64);
}

TEST(TST_CHUNK, tst_get_and_set_next){
    auto cp = (mChunkPtr)(envp->p);
    cp->set_head(97);

    auto np = cp->nxt_chunk();
    cp->set_head(97);

    cout<<(void*)cp<<endl;
    cout<<(void*)np<<endl;

    cp->set_next_allocated(np); /**/
    cp->set_prev_allocated(nullptr);
    np->set_next_allocated(nullptr);
    np->set_prev_allocated(cp);

    EXPECT_EQ(cp->get_next_allocated(), np);
    EXPECT_EQ((char*)(cp->get_next_allocated()), envp->p + 96);
    EXPECT_EQ(np->get_prev_allocated(), cp);
    EXPECT_EQ((char*)(np->get_prev_allocated()), envp->p );
}



int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(envp);
    return RUN_ALL_TESTS();
}