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
class Arena;
struct gcmallocPar;
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
    friend class Bins;
    friend class Arena;
    friend class gcmalloc;
public:
    /*--------------- Some size operations ---------------*/
    /* Get size, ignoring use bits */
    [[nodiscard]] size_t get_size() const;
    [[nodiscard]] size_t get_prev_size() const;
    [[nodiscard]] size_t get_size_field() const;
    [[nodiscard]] size_t get_prev_size_field() const;
    [[nodiscard]] size_t get_valid_size() const;

    /* foot is next chunk's prev_size */
    void set_head(size_t sizeAndBits);
    void set_foot(size_t sz);
    void set_head_size(size_t sz);
    void set_prev_size(size_t sz);

public:
    /*--------------- Physical chunk operations ---------------*/
    [[nodiscard]] char *this_chunk() const;
    [[nodiscard]] mChunkPtr nxt_chunk() const;
    [[nodiscard]] mChunkPtr pre_chunk() const;

public:
    /*--------------- Physical bits operations ---------------*/
    [[nodiscard]] bool prev_inuse() const;
    [[nodiscard]] bool is_mmapped() const;
    [[nodiscard]] bool is_in_non_main_arena() const;
    [[nodiscard]] bool is_marked() const;

    /* This chunk's inuse information is stored in next chunk */
    bool inuse();
    void set_inuse();
    void clear_inuse();

    /* gc marked */
    void set_marked();
    void clear_marked();

public:
    /* conversion from malloc headers to user pointers, and back */
    [[nodiscard]] void *chunk2mem() const;

    [[nodiscard]] bool is_aligned() const;
    mHeapPtr belonged_heap();
    void unlink(mChunkPtr bck, mChunkPtr fwd);

    Chunk() : prev_size(0), size(0), fd(nullptr), bk(nullptr),
              fd_nextsize(nullptr), bk_nextsize(nullptr) {}

public:
    /* allocated ops */
    [[nodiscard]] mChunkPtr get_next_allocated() const;
    void set_next_allocated(mChunkPtr p);

    /* allocated ops */
    [[nodiscard]] mChunkPtr get_prev_allocated() const;
    void set_prev_allocated(mChunkPtr p);


private:
    size_t prev_size;   /* Size of previous chunk (if free).  */
    size_t size;        /* Size in bytes, including overhead. */

    mChunkPtr fd;
    mChunkPtr bk;

    /* Only used for large blocks: pointer to next larger size.  */
    mChunkPtr fd_nextsize;
    mChunkPtr bk_nextsize;
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
