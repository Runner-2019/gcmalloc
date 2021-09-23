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
using ::testing::Test;
class TST_CHUNK : public ::testing::Test {
public:

};

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


int main(int argc, char* argv[]){
    testing::InitGoogleTest(&argc, argv);
    1 + 2;
    return RUN_ALL_TESTS();
}