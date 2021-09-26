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
#ifndef GCMALLOC_GCMALLOC_H
#define GCMALLOC_GCMALLOC_H
#include "Arena.h"
#include "concurrency.h"
#include <functional>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

inline void MALLOC_FAILURE_ACTION() {
    errno = ENOMEM;
}

/* function pointer */
using mallocFuncType = std::function<void *(size_t sz)>;
using freeFuncType = std::function<void(void *)>;


struct gcmallocPar {
    /* for fast visit these fields, we use 'public'. Is friend class better ? */
    /* Tunable parameters */
    unsigned long trim_threshold;
    size_t mmap_threshold;
    size_t top_pad;

    /* Memory map  support */
    int n_mmaps;
    int n_mmaps_max;
    int max_n_mmaps;

    /*
        the mmap_threshold is dynamic, until the user sets
        it manually, at which point we need to disable any
        dynamic behavior.
    */
    int no_dyn_threshold;

    /* Cache malloc_getpagesize */
    unsigned int pagesize;

    /* Statistics */
    size_t mmapped_mem;
    size_t max_mmapped_mem;

    /* First address handed out by sbrk.  */
    char *sbrk_base;
};


class TST_GCMALLOC;

class gcmalloc {

public:
    void *malloc(size_t bytes);
    void free(void *mem);
    gcmalloc();


private:
    /* arena ops */
    mArenaPtr get_arena(size_t sz);
    mArenaPtr get_arena2(mArenaPtr a_tsd, size_t sz);
    mArenaPtr createArena(size_t sz);
    mArenaPtr main_arena();

private:
    /* init all */
    void gcmalloc_init_minimal();
    void init();

    /* operate user's size */
    bool request_out_of_range(size_t sz);
    size_t req2size(size_t sz);
    size_t checked_req2size(size_t sz);

    /* ----------- Routines dealing with system allocation -------------- */
    void *malloc_from_sys(size_t sz, mArenaPtr ap);
    int trim_to_sys(size_t pad, mArenaPtr ap);
    void munmap_chunk(mChunkPtr p);


#ifdef GCMALLOC_DEBUG
    /* for unit test*/
private:
    /* Debugging support */
    void check_chunk(mArenaPtr ap, mChunkPtr cp);
    void check_free_chunk(mArenaPtr ap, mChunkPtr cp);
    void check_inuse_chunk(mArenaPtr ap, mChunkPtr cp);
    void check_remalloced_chunk(mArenaPtr ap, mChunkPtr cp, size_t sz);
    void check_malloced_chunk(mArenaPtr ap, mChunkPtr cp, size_t sz);
    void check_malloc_state(mArenaPtr ap);

    static int perturb_byte;
    void alloc_perturb(void *p, size_t n);
    void free_perturb(void *p, size_t n);

#endif


private:
    static mArenaPtr arena_for_chunk(mChunkPtr cp);
    void *_int_malloc(mArenaPtr ap, size_t sz);
    void _int_free(mArenaPtr ap, void *mem);
    void malloc_consolidate(mArenaPtr ap);
    void malloc_printerr(int action, const char *str, void *ptr);

private:
    gcmallocPar gcmp;
    Arena _main_arena;
    Tsd arena_key;
    Mutex list_lock;

private:
    mallocFuncType malloc_hook; // the real malloc version we used now
    mallocFuncType save_malloc_hook;
    freeFuncType free_hook;
    freeFuncType save_free_hook;
};

#endif //GCMALLOC_GCMALLOC_H
