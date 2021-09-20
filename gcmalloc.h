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
#include "arena.h"
#include "config.h"
#include <functional>
#include <fcntl.h>
#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
static int dev_zero_fd = -1; /* Cached file descriptor for /dev/zero. */
inline void* MMAP(void* addr, size_t size, int prot, int flags){
    if(dev_zero_fd < 0)
        dev_zero_fd = open("/dev/zero",O_RDWR);
    return mmap(addr,size,prot, flags,dev_zero_fd,0);
}
#else
inline void *MMAP(void *addr, size_t size, int prot, int flags) {
    return mmap(addr, size, prot, flags | MAP_ANONYMOUS, -1, 0);
}
#endif


/* Forward declaration*/


/* function pointer */
using mallocFuncType = std::function<void *()>;
using freeFuncType = std::function<void()>;






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


class gcmalloc {

private:
    void init();


    /*
        get() acquires an arena and locks the corresponding mutex.
        First, try the one last locked successfully by this thread.
        (This is the common case.)  Then, loop once over the circularly
        linked list of arenas.  If no arena is readily available,
        create a new one.In this latter case, `size' is just a hint as
        to how much memory will be required immediately in the new arena .
     */
    mArenaPtr get_arena(size_t sz);
    mArenaPtr get_arena2(mArenaPtr  a_tsd, size_t sz);

    /* Create a new arena with initial size "size".  */




private:
    void gcmalloc_init_minimal();
    mArenaPtr createArena(size_t sz);



private:
    bool initialized;  /* whether being initialized */
    gcmallocPar gcmp;
    mallocFuncType malloc_hook; // the real malloc version we used now
    mallocFuncType save_malloc_hook;
    freeFuncType free_hook;
    freeFuncType save_free_hook;

    static Arena main_arena;

private:
    /* Support thread */
    static Tsd arena_key;
    static Mutex list_lock;
};

#endif //GCMALLOC_GCMALLOC_H
