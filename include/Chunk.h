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
#ifndef GCMALLOC_CHUNK_H
#define GCMALLOC_CHUNK_H
#include "config.h"
#include <cstddef>
#include <cassert>

/*
    Forward declaration
    Chunk - Chunk belongs to all.    Manage the real memory which allocated to users or freed.
    Heap  - Heap belongs to Arena.   Manage allocated and freed Chunk.
    Bins  - Bins belongs to Arena.   Manage the freed chunk.
    Arena - Arena belongs to thread. Manage all。
*/
class Chunk;
class Heap;
class Bins;
class FastBins;
class BinMap;
struct Arena;
using mChunkPtr = Chunk *;
using mHeapPtr = Heap *;
using mBinPtr = mChunkPtr;
using mFastbinPtr = mChunkPtr;
using mArenaPtr = Arena *;

/* --------------------------------- Chunk ---------------------------------*/
const size_t PREV_INUSE = 0x1;
const size_t IS_MMAPPED = 0x2;
const size_t NON_MAIN_ARENA = 0x4;
const size_t GC_MARKED = 0x8;
const size_t allBits = (PREV_INUSE | IS_MMAPPED | NON_MAIN_ARENA | GC_MARKED);


/* change mem to the Chunk it belonged to*/
inline mChunkPtr mem2Chunk(void *mem) {
    return reinterpret_cast<mChunkPtr>((char *) (mem) - 2 * SIZE_SZ);
}

/* treat the space(begins at p + offset) as a Chunk*/
inline mChunkPtr offset2Chunk(void *p, size_t offset) {
    return (mChunkPtr) ((char *) p + offset);
}


class Chunk {
public:
    /* conversion from malloc headers to user pointers, and back */
    void *chunk2mem();

    bool is_aligned();
    mHeapPtr belonged_heap();
    friend class Bins;
//    friend class ;
    friend class gcmalloc;
public:
    /*--------------- Some size operations ---------------*/
    /* Get size, ignoring use bits */
    [[nodiscard]] size_t get_size() const;

    /* Set size at head */
    void set_head_size(size_t sz); // not
    void set_head(size_t sizeAndBits); // yes
    void set_foot(size_t sz);


public:
    /*--------------- Physical chunk operations ---------------*/
    char *this_chunk();
    mChunkPtr nxt_chunk();
    mChunkPtr pre_chunk();

public:
    /*--------------- Physical bits operations ---------------*/
    bool prev_inuse();
    bool is_mmapped();
    bool is_in_non_main_arena();

    /* This chunk's inuse information is stored in next chunk */
    bool inuse();
    void set_inuse();
    void clear_inuse();

public:
    void unlink(mChunkPtr bk, mChunkPtr fd);


private:
    size_t prev_size;   /* Size of previous chunk (if free).  */
    size_t size;        /* Size in bytes, including overhead. */

    Chunk *fd;
    Chunk *bk;

    /* Only used for large blocks: pointer to next larger size.  */
    Chunk *fd_nextsize;
    Chunk *bk_nextsize;
};

/* -----  some ops for bins ---*/
inline bool in_smallbin_range(size_t sz) { return sz < SMALL_BINS_MAX_SIZE; }
inline size_t smallbin_idx(size_t sz) /* logic index */ { return sz >> 4; }
inline size_t largebin_idx(size_t sz) {
    size_t ret = (((sz >> 6) <= 48) ? 48 + (sz >> 6) : \
     ((sz >> 9) <= 20) ? 91 + (sz >> 9) : \
     ((sz >> 12) <= 10) ? 110 + (sz >> 12) : \
     ((sz >> 15) <= 4) ? 119 + (sz >> 15) : \
     ((sz >> 18) <= 2) ? 124 + (sz >> 18) : \
                                            126);
    return ret;
}
inline size_t bin_idx(size_t sz) { return in_smallbin_range(sz) ? smallbin_idx(sz) : largebin_idx(sz); }


#endif //GCMALLOC_CHUNK_H
