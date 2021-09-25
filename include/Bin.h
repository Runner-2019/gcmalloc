/*
  --------------------------------------
     ╭╩══╮╔══════╗╔══════╗╔══════╗ 
    ╭    ╠╣      ╠╣      ╠╣      ╣
    ╰⊙══⊙╯╚◎════◎╝╚◎════◎╝╚◎════◎╝
  --------------------------------------
  @date:   2021-九月-20
  @author: xiaomingZhang2020@outlook.com
  --------------------------------------
*/
#ifndef GCMALLOC_BIN_H
#define GCMALLOC_BIN_H
#include "Chunk.h"


/* placed the allocated chunk */
class UsedBin{
public:
    void insert(mChunkPtr p);
    void remove(mChunkPtr p);
    mChunkPtr get_used_head();


private:
    mChunkPtr __used_head; /* a dummy head */
};


/*
    Bins has three parts.
    The first is unsorted bin and the second is small bin and the left is large bin.
*/

class Bins {
public:
    mBinPtr bin_at(size_t i);
    mBinPtr next_bin(mBinPtr bin);
    mChunkPtr chunk_at_first(mBinPtr bin);
    mChunkPtr chunk_at_last(mBinPtr bin);

private:
    /* Normal bins packed as described above */
    mBinPtr bins[NUM_BINS * 2 - 2];
};

/*
    FastBins：single linked. The smallest size of FastBins is 8 bytes.
    Then 8 bytes added between adjacent bins. The maximum of FastBins is
    DEFAULT_MAX_FAST. The number of bins is NUM_FAST_BINS.
*/

class FastBins {
public:
    size_t index(size_t sz);
    mBinPtr & operator[](size_t i){
        return fastBins[i];
    }
private:
    mBinPtr fastBins[NUM_FAST_BINS];
};


/*
  BinMap
    To help compensate for the large number of bins, a one-level index
    structure is used for bin-by-bin searching. BinMap is a
    bitvector recording whether bins are definitely empty so they can
    be skipped over during traversals.  The bits are NOT always
    cleared as soon as bins are empty, but instead only
    when they are noticed to be empty during traversal in malloc.
*/

class BinMap {
public:
    size_t idx2block(size_t i);
    size_t idx2bit(size_t i);
    void mark_bin(size_t i);
    void unmark_bin(size_t i);
    unsigned int get_binmap(size_t i);

    unsigned int& operator[](size_t i){
        return binmap[i];
    }

private:
    unsigned int binmap[BIN_MAP_SIZE]{0};     /* Bitmap of bins */
};


#endif //GCMALLOC_BIN_H
