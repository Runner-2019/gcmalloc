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
    Chunk - Chunk belongs to all.    Manage the real memory allocated to users or freed.
    Heap  - Heap belongs to Arena.   Manage allocated and freed Chunk.
    Bins  - Bins belongs to Arena.   Manage the freed chunk.
    Arena - Arena belongs to thread. Manage all。
*/
class Chunk;
class Heap;
class Bin;
struct Arena;
using mChunkPtr = Chunk *;
using mHeapPtr = Heap *;
using mBinPtr = mChunkPtr;
using mFastbinPtr = mChunkPtr;
using mArenaPtr = Arena *;

/* --------------------------------- Chunk ---------------------------------
1. Boundary tag:
    Chunks of memory are maintained using a `boundary tag' method.
    Sizes of free chunks are stored both in the front of each chunk and at the end.

2. why use boundary tag?
    This makes consolidating fragmented chunks into bigger chunks very fast.

3. What's Chunks look like?
(1) An allocated chunk looks like this:
    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of previous chunk, if allocated            | |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of chunk, in bytes                       |M|P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             User data starts here...                          .
            .                                                               .
            .             (some bytes)                                      .
            .                                                               |
 nxtchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of chunk                                     |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    "chunk"     : the front of the Chunk for the purpose of most of the malloc code
    "mem"       : the pointer that is returned to the user.
    "nxtchunk"  : the beginning of the next contiguous chunk.

(2) Free chunks are stored in circular doubly-linked lists, and look like this:
    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of previous chunk                            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `head:' |             Size of chunk, in bytes                         |P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Forward pointer to next chunk in list             |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Back pointer to previous chunk in list            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Unused space (may be 0 bytes long)                .
            .                                                               .
            .                                                               |
 nxtchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `foot:' |             Size of chunk, in bytes                           |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

/* size field is or'ed with PREV_INUSE when previous adjacent chunk in use */
const size_t PREV_INUSE = 0x1;
const size_t IS_MMAPPED = 0x2;
const size_t NON_MAIN_ARENA = 0x4;
const size_t allBits = (PREV_INUSE | IS_MMAPPED | NON_MAIN_ARENA);


/* change mem to the Chunk it belonged to*/
inline mChunkPtr mem2chunk(void *mem) {
    return reinterpret_cast<mChunkPtr>((char *) (mem) - 2 * SIZE_SZ);
}

/* treat the space(begins at p + offset) as a Chunk*/
inline mChunkPtr offset2Chunk(void *p, size_t offset) {
    return (mChunkPtr) ((char *) p + offset);
}
//
//size_t inuse_bit_at_offset(void *p, size_t sz) {
//    return ((offset2Chunk(p,sz))->size & PREV_INUSE);
//}
//void set_inuse_bit_af_offset(size_t sz);
//void clear_inuse_bit_af_offset(size_t sz);


class Chunk {
public:
    size_t prev_size;  /* Size of previous chunk (if free).  */
    size_t size;       /* Size in bytes, including overhead. */

    struct Chunk *fd;
    struct Chunk *bk;

    /* Only used for large blocks: pointer to next larger size.  */
    struct Chunk *fd_nextsize;
    struct Chunk *bk_nextsize;

public:
    /* conversion from malloc headers to user pointers, and back */
    void *chunk2mem();

    bool is_aligned();

    mHeapPtr belonged_heap();

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
    bool is_mmaped();
    bool is_in_non_main_arena();

    /* This chunk's inuse information is stored in next chunk */
    bool inuse();
    void set_inuse();
    void clear_inuse();

public:
    void unlink(mChunkPtr bk, mChunkPtr fd);

};


/* -----  some ops for bins ---*/
inline bool in_smallbin_range(size_t sz) { return sz < SMALLBIN_MAX_SIZE; }
inline size_t smallbin_idx(size_t sz) { return sz >> 4; }
inline size_t largebin_idx(size_t sz) {
    size_t ret = 0;
    if (sz >> 6 <= 48)
        ret += 48 + (sz >> 6);
    if (sz >> 9 <= 20)
        ret += 91 + (sz >> 9);
    if (sz >> 12 <= 10)
        ret += 110 + (sz >> 12);
    if (sz >> 15 <= 4)
        ret += 119 + (sz >> 15);
    if (sz >> 18 <= 2)
        ret += 124 + (sz >> 18);
    else
        ret += 126;
    return ret;
}
inline size_t bin_idx(size_t sz) { return in_smallbin_range(sz) ? smallbin_idx(sz) : largebin_idx(sz); }


#endif //GCMALLOC_CHUNK_H
