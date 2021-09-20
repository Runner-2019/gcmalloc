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
#include "gcmalloc.h"
mArenaPtr gcmalloc::get_arena(size_t sz) {
    auto arena = (mArenaPtr) (arena_key.get());
    if (arena != nullptr && !(arena->mutex).trylock()) {
        return arena;
    } else {
        return (mArenaPtr) get_arena2(arena, sz);
    }
}


mArenaPtr gcmalloc::get_arena2(mArenaPtr a_tsd, size_t sz) {
    mArenaPtr a;

    if (!a_tsd)
        a = a_tsd = &main_arena;
    else {
        a = a_tsd->next;
        if (!a) {
            /* This can only happen while initializing the new arena. */
            main_arena.mutex.lock();
            return &main_arena;
        }
    }

    /* Check the global, circularly linked list for available arenas. */
    bool retried = false;
    repeat:
    do {
        if (a->mutex.trylock()) { /* Lock success */
            /* If this is the second loop, the list_lock must be locked */
            if (retried)
                list_lock.unlock();
            arena_key.set(a);
            return a;
        }
        a = a->next;
    } while (a != a_tsd);

    /*
        If not even the list_lock can be obtained, try again.  This can
        happen during `atfork', or for example on systems where thread
        creation makes it temporarily impossible to obtain any locks.
    */
    if (!retried && !list_lock.trylock()) {
        /* We will block to not run in a busy loop.  */
        list_lock.lock();

        /* Since we blocked there might be an arena available now.  */
        retried = true;
        a = a_tsd;
        goto repeat;
    }

    /* Nothing immediately available, so generate a new arena.  */
    a = createArena(sz);
    if (a) {
        arena_key.set(a);
        (a->mutex).lock();

        /* Add the new arena to the global list.  */
        a->next = main_arena.next;
        main_arena.next = a;
    }
    list_lock.unlock();
    return a;
}


void gcmalloc::gcmalloc_init_minimal() {
    gcmp.top_pad = DEFAULT_TOP_PAD;
    gcmp.n_mmaps_max = DEFAULT_MMAP_MAX;
    gcmp.mmap_threshold = DEFAULT_MMAP_THRESHOLD;
    gcmp.trim_threshold = DEFAULT_TRIM_THRESHOLD;
    gcmp.pagesize = DEFAULT_PAGESIZE;
}

void gcmalloc::init() {
    if (initialized) //already initialize
        return;
    initialized = false;
    gcmalloc_init_minimal();

    /*
        With some threads implementations, creating thread-specific data
        or initializing a mutex may call malloc() itself.  Provide a
        simple starter version (now it won't work since first release).
     */
    main_arena.next = &main_arena;
    arena_key.set(static_cast<void *>(&main_arena));

    // we can register fork in there in next release
    // we can read user's input to modify config in there in next release
}


mArenaPtr gcmalloc::createArena(size_t sz) {
    mArenaPtr a;
    Heap *h;
    char *ptr;
    unsigned long misalign;

    h = new Heap(sz + (sizeof(*h) + sizeof(*a) + GCMALLOC_ALIGNMENT),
                 gcmp.top_pad);

    if (!h->this_heap) {
        /* Maybe size is too large to fit in a single heap.  So, just try
           to create a minimally-sized arena and let _int_malloc() attempt
           to deal with the large request via mmap_chunk().  */
        h = new Heap(sizeof(*h) + sizeof(*a) + GCMALLOC_ALIGNMENT, gcmp.top_pad);
        if (!h->this_heap)
            return nullptr;
    }
    a = h->ar_ptr = (mArenaPtr) (h + 1);
    a->init(false);
    /*a->next = NULL;*/
    a->system_mem = a->max_system_mem = h->size;


    /* Set up the top chunk, with proper alignment. */
    ptr = (char *) (a + 1);

    misalign = (unsigned long) (ptr - 2 * SIZE_SZ) & GCMALLOC_ALIGN_MASK;
    if (misalign > 0)
        ptr += GCMALLOC_ALIGNMENT - misalign;

    a->topChunk = mem2chunk(ptr);
    a->topChunk->set_head(((char *) h + h->size - ptr) | PREV_INUSE);
    return a;
}
