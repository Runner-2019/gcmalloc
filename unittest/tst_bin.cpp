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
#include "../include/Bin.h"
using namespace std;

class TST_BIN : public ::testing::Test {
};

TEST(TST_BIN, tst_bins_bin_at) {
    Bins bin;
//    EXPECT_EQ(bin.bin_at(63),122 + 2);
}

TEST(TST_BIN, tst_bins_next_bin) {
    Bins bin;
    EXPECT_EQ(bin.bin_at(2), bin.next_bin(bin.bin_at(1)));
}

TEST(TST_BIN, tst_fastbin_index) {
    FastBins fb;
    ASSERT_EQ(fb.index(16), 0);
}

TEST(TST_BIN, tst_binmap_idx2block) {
    BinMap binmap;
    EXPECT_EQ(binmap.idx2block(0), 0);  // 0-31 == the 0th block, 32 - 63 == the first blk
    EXPECT_EQ(binmap.idx2block(31), 0); // 0-31 == the 0th block, 32 - 63 == the first blk
    EXPECT_EQ(binmap.idx2block(32), 1); // 0-31 == the 0th block, 32 - 63 == the first blk
    EXPECT_EQ(binmap.idx2block(32), 1); // 0-31 == the 0th block, 32 - 63 == the first blk
}

TEST(TST_BIN, tst_binmap_idx2bit) {
    BinMap binmap;
    EXPECT_EQ(binmap.idx2bit(0), 1);
    EXPECT_EQ(binmap.idx2bit(1), 2);
    EXPECT_EQ(binmap.idx2bit(2), 4);
    EXPECT_EQ(binmap.idx2bit(3), 8);
    EXPECT_EQ(binmap.idx2bit(4), 16);
}

TEST(TST_BIN, tst_mark_and_unmark_bin) {
    BinMap binmap;
    binmap.mark_bin(0);
    EXPECT_EQ(binmap.get_binmap(0), 1);

    binmap.unmark_bin(0);
    EXPECT_EQ(binmap.get_binmap(0), 0);

    binmap.mark_bin(0);
    EXPECT_EQ(binmap.get_binmap(0), 1);

}

TEST(TST_BIN, tst_used_bin_size){
    UsedBin ub;
    EXPECT_EQ(MINSIZE, 64);
    EXPECT_EQ(sizeof(ub), 128);
    EXPECT_EQ((char*)ub.head(), (char*)&ub);
    EXPECT_EQ((char*)ub.tail(), (char*)&ub + 64);
}

TEST(TST_BIN, tst_used_bin){
    UsedBin ub;
    EXPECT_EQ(ub.head()->get_next_allocated(), ub.tail());
    EXPECT_EQ(ub.tail()->get_prev_allocated(), ub.head());

    void* p = mmap(nullptr,200,PROT_READ|PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS, -1,0);
    mChunkPtr cp1 = (mChunkPtr)p, cp2 = offset2Chunk(cp1, 96);
    cp1->set_head(97);
    cp2->set_head(97);

    ub.insert(cp1);
    EXPECT_EQ(ub.head()->get_next_allocated(), cp1);
    EXPECT_EQ(ub.tail()->get_prev_allocated(), cp1);

    ub.insert(cp2);
    EXPECT_EQ(ub.head()->get_next_allocated(), cp1);
    EXPECT_EQ(cp1->get_next_allocated(), cp2);
    EXPECT_EQ(cp2->get_next_allocated(), ub.tail());
    EXPECT_EQ(cp1->get_prev_allocated(),ub.head());
    EXPECT_EQ(cp2->get_prev_allocated(),cp1);
    EXPECT_EQ(ub.tail()->get_prev_allocated(),cp2);

    ub.remove(cp2);
    EXPECT_EQ(ub.head()->get_next_allocated(), cp1);
    EXPECT_EQ(cp1->get_next_allocated(), ub.tail());
    EXPECT_EQ(ub.tail()->get_prev_allocated(), cp1);
    EXPECT_EQ(cp1->get_prev_allocated(), ub.head());

    ub.remove(cp1);
    EXPECT_EQ(ub.head()->get_next_allocated(), ub.tail());
    EXPECT_EQ(ub.tail()->get_prev_allocated(), ub.head());

}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}