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
#include "Bin.h"
#include "Heap.h"


/* malloc state */
class Arena {
public:
    int heap_trim(Heap *heap, size_t pad);
    void init(bool is_main_arena);
    mChunkPtr initial_top();

    /* have fast chunk or not */
    bool have_fast_chunks();
    void clear_fast_chunks();
    void set_fast_chunks();

    /* contiguous or not */
    bool contiguous();
    bool non_contiguous();
    bool set_contiguous();
    void set_non_contiguous();


    /* The otherwise unindexable 1-bin is used to hold unsorted chunks. */
    mChunkPtr unsorted_chunks();


public:
    Mutex mutex;
    int flags;
    FastBin fast_bins;
    mChunkPtr topChunk;
    mChunkPtr last_remainder;
    Bins bins;
    BinMap binmap;
    Arena *next;
    size_t system_mem;
    size_t max_system_mem;
};



extern void *MMAP(void *addr, size_t size, int prot, int flags);


#endif //GCMALLOC_ARENA_H
