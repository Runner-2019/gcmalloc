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
    /* have fast chunk or not */
    [[nodiscard]] bool have_fast_chunks() const;
    void clear_fast_chunks();
    void set_fast_chunks();

    /* contiguous or not */
    [[nodiscard]] bool contiguous() const;
    [[nodiscard]] bool non_contiguous() const;
    void set_contiguous();
    void set_non_contiguous();

public:
    /* The otherwise un-index-able 1-bin is used to hold unsorted chunks. */
    mChunkPtr unsorted_chunks();
    mChunkPtr initial_top();

    void init(bool is_main_arena);
    int heap_trim(Heap *heap, size_t pad);

    /* mutex ops */
    void init_this_arena_lock();
    void lock_this_arena();
    void unlock_this_arena();
    bool trylock_this_arena();

public:
    friend class gcmalloc;
    Arena() noexcept = default;
private:
    Mutex m_mutex;
    int flags;
    FastBins fast_bins;
    mChunkPtr top_chunk;
    mChunkPtr last_remainder;
    Bins bins;
    BinMap binmap;
    mArenaPtr next;
    size_t system_mem;
    size_t max_system_mem;
};

#endif //GCMALLOC_ARENA_H
