/*
  --------------------------------------
     ╭╩══╮╔══════╗╔══════╗╔══════╗ 
    ╭    ╠╣      ╠╣      ╠╣      ╣
    ╰⊙══⊙╯╚◎════◎╝╚◎════◎╝╚◎════◎╝
  --------------------------------------
  @date:   2021-九月-19
  @author: xiaomingZhang2020@outlook.com
  --------------------------------------
*/
#ifndef GCMALLOC_ARENA_H
#define GCMALLOC_ARENA_H
#include <sys/mman.h>
#include "config.h"
#include "concurrency.h"
#include "Chunk.h"

/*
    Forward declaration
    Chunk - Chunk belongs to heap.
    Heap  - Heap belongs to arena.
    Arena - Arena belongs to thread.
*/
struct Heap;
struct Arena;

using mHeapPtr = Heap *;
using mArenaPtr = Arena *;
using mbinPtr = mChunkPtr;

/* malloc state */
class Arena {
public:
    int heap_trim(Heap *heap, size_t pad);
    static void unlink(mChunkPtr p, mChunkPtr bk, mChunkPtr fd);
    void init(bool is_main_arena);
    mbinPtr bin_at(size_t i);
    void initial_top();
    void set_non_contiguous();

public:
    /* Serialize access.  */
    Mutex mutex;

    /* Flags (formerly in max_fast).  */
    int flags;

    /* Fast bins */
    mbinPtr fastBins[NFASTBINS];

    /* Base of the topmost chunk -- not otherwise kept in a bin*/
    mChunkPtr topChunk;

    /* The remainder from the most recent split of a small request */
    mChunkPtr last_remainder;

    /* Normal bins packed as described above */
    mbinPtr bins[NBINS * 2 - 2];

    /* Bitmap of bins */
    unsigned int binmap[BINMAPSIZE];

    /* Linked list */
    Arena *next;

    /* Memory allocated from the system in this arena.*/
    size_t system_mem;
    size_t max_system_mem;

};

/* heap */
struct Heap {
public:
    /* Create a new heap. Size is automatically rounded up to a multiple of the page size */
    mHeapPtr create(size_t sz, size_t top_pad);

    /* Delete a heap */
    ~Heap();

    /* Grow a heap. Size is automatically rounded up to a multiple of the page size */
    int grow(size_t sz);

    /* Shrink a heap.  */
    int shrink(size_t sz);

    Heap(size_t sz, size_t top_pad) {
        this_heap = create(sz, top_pad);
    }

public:
    mArenaPtr ar_ptr{};      /* arena for this heap */
    mHeapPtr this_heap;      /* this heap */
    mHeapPtr prev_heap{};    /* Previous heap. */
    size_t size;             /* Current size in bytes.*/
    size_t protect_size;     /* Size in bytes that has been mprotected PROT_READ|PROT_WRITE.*/
    char m_pad[SIZE_SZ];           /* Pad to align*/

    /*
        If consecutive mmap (0, HEAP_MAX_SIZE << 1, ...) calls return decreasing
        addresses as opposed to increasing, new_heap would badly fragment the
        address space.  In that case remember the second HEAP_MAX_SIZE part
        aligned to HEAP_MAX_SIZE from last mmap (0, HEAP_MAX_SIZE << 1, ...)
        call (if it is already aligned) and try to reuse it next time.  We need
        no locking for it, as kernel ensures the atomicity for us - worst case
        we'll call mmap (addr, HEAP_MAX_SIZE, ...) for some value of addr in
        multiple threads, but only one will succeed.
    */
    static char *aligned_heap;
};


extern void *MMAP(void *addr, size_t size, int prot, int flags);


#endif //GCMALLOC_ARENA_H
