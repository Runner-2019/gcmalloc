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
#include "include/gcmalloc.h"


/* first of all, we will initialize the gcmalloc*/
gcmalloc::gcmalloc() : gcmp{}, perturb_byte(0), malloc_hook(nullptr), free_hook(nullptr) {
    init();
}

void gcmalloc::gcmalloc_init_minimal() {
    gcmp.top_pad = DEFAULT_TOP_PAD;
    gcmp.n_mmaps_max = DEFAULT_MMAP_MAX;
    gcmp.mmap_threshold = DEFAULT_MMAP_THRESHOLD;
    gcmp.trim_threshold = DEFAULT_TRIM_THRESHOLD;
    gcmp.pagesize = DEFAULT_PAGESIZE;
}

void gcmalloc::init() {
    gcmalloc_init_minimal();
    memset(&_main_arena, 0, sizeof(_main_arena));
    /*
        With some threads implementations, creating thread-specific data
        or initializing a mutex may call malloc() itself.  Provide a
        simple starter version (now it won't work since first release).
     */
    main_arena()->init(THIS_IS_MAIN_ARENA);
    main_arena()->next = main_arena();
    arena_key.set(main_arena());
    // we can register fork in there in next release
    // we can read user's input to modify config in there in next release
}


mArenaPtr gcmalloc::get_arena(size_t sz) {
    auto arena = (mArenaPtr) (arena_key.get());
    if (arena != nullptr && arena->trylock_this_arena()) {
        return arena;
    } else {
        return (mArenaPtr) get_arena2(arena, sz);
    }
}


mArenaPtr gcmalloc::get_arena2(mArenaPtr a_tsd, size_t sz) {
    mArenaPtr a = a_tsd->next; /* In our version, a never equals to NULL */

    /* Check the global, circularly linked list for available arenas. */
    bool retried = false;
    repeat:
    do {
        if (a->trylock_this_arena()) { /* Lock success */
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
        a->lock_this_arena();

        /* Add the new arena to the global list.  */
        a->next = main_arena()->next;
        main_arena()->next = a;
    }
    list_lock.unlock();
    return a;
}

mArenaPtr gcmalloc::createArena(size_t sz) {
    mArenaPtr a;
    Heap *h;
    char *ptr;
    unsigned long misalign;

    h = new_heap(sz + (sizeof(*h) + sizeof(*a) + GCMALLOC_ALIGNMENT),
                 gcmp.top_pad);

    if (!h) {
        /*
            Maybe size is too large to fit in a single heap.  So, just try
            to create a minimally-sized arena and let _int_malloc() attempt
            to deal with the large request via mmap_chunk().
        */
        h = new_heap(sizeof(*h) + sizeof(*a) + GCMALLOC_ALIGNMENT, gcmp.top_pad);
        if (!h)
            return nullptr;
    }

    /* Arena is physical-closing to heap */
    a = h->ar_ptr = (mArenaPtr) (h + 1);
    a->init(THIS_IS_NON_MAIN_ARENA);
    a->init_this_arena_lock();

    /*a->next = nullptr;*/
    a->system_mem = a->max_system_mem = h->size;


    /* Set up the top chunk, with proper alignment. */
    ptr = (char *) (a + 1);
    misalign = ((unsigned long) (ptr + 2 * SIZE_SZ)) & GCMALLOC_ALIGN_MASK;
    if (misalign > 0)
        ptr += GCMALLOC_ALIGNMENT - misalign;

    a->top_chunk = (mChunkPtr) (ptr);
    a->top_chunk->set_head(((char *) h + h->size - ptr) | PREV_INUSE);
    return a;
}

mArenaPtr gcmalloc::main_arena() {
    return &_main_arena;
}

/* mod user's size */
bool gcmalloc::request_out_of_range(size_t sz) {
    return (unsigned long) sz >= (unsigned long) (size_t) (-2 * MINSIZE);
}

size_t gcmalloc::req2size(size_t sz) {
    return (sz + CHUNK_OVER_HEAD + GCMALLOC_ALIGN_MASK < MINSIZE ?
            MINSIZE : \
            (sz + CHUNK_OVER_HEAD + GCMALLOC_ALIGN_MASK) & ~GCMALLOC_ALIGN_MASK);
}

size_t gcmalloc::checked_req2size(size_t sz) {
    if (request_out_of_range(sz)) {
        MALLOC_FAILURE_ACTION();
        return 0;
    }
    return req2size(sz);
}


mArenaPtr gcmalloc::arena_for_chunk(mChunkPtr cp) {
    return cp->is_in_non_main_arena() ? cp->belonged_heap()->ar_ptr : main_arena();
}


void gcmalloc::check_chunk(mArenaPtr ap, mChunkPtr cp) const {
    unsigned long sz = cp->get_size();
    /* min and max possible addresses assuming contiguous allocation */
    char *max_address = (char *) (ap->top_chunk) + ap->top_chunk->get_size();
    char *min_address = max_address - ap->system_mem;

    if (!cp->is_mmapped()) {
        /* Has legal address ... */
        if (cp != ap->top_chunk) {
            if (ap->contiguous()) {
                assert(((char *) cp) >= min_address);
                assert(((char *) cp + sz) <= ((char *) (ap->top_chunk)));
            }
        } else {
            /* top size is always at least MINSIZE */
            assert((unsigned long) (sz) >= MINSIZE);
            /* top predecessor always marked inuse */
            assert(cp->prev_inuse());
        }
    } else {
        /* address is outside main heap  */
        if (ap->contiguous() && ap->top_chunk != ap->initial_top()) {
            assert(((char *) cp) < min_address || ((char *) cp) >= max_address);
        }

        /* chunk is page-aligned */
        assert(((cp->prev_size + sz) & (gcmp.pagesize - 1)) == 0);

        /* mem is aligned */
        assert((cp->is_aligned()));
    }
}

void gcmalloc::check_free_chunk(mArenaPtr ap, mChunkPtr cp) {
    size_t sz = cp->get_size();
    mChunkPtr next = offset2Chunk(cp, sz);

    check_chunk(ap, cp);

    /* Chunk must claim to be free ... */
    assert(!cp->inuse());
    assert (!cp->is_mmapped());

    /* Unless a special marker, must have OK fields */
    if ((unsigned long) (sz) >= MINSIZE) {
        assert((sz & GCMALLOC_ALIGN_MASK) == 0);
        assert(cp->is_aligned());

        /* ... matching footer field */
        assert(next->get_prev_size() == sz);

        /* ... and is fully consolidated */
        assert(cp->prev_inuse());
        assert (next == ap->top_chunk || next->inuse());

        /* ... and has minimally sane links */
        assert(cp->fd->bk == cp);
        assert(cp->bk->fd == cp);
    } else /* markers are always of size SIZE_SZ */
        assert(sz == SIZE_SZ);
}

void gcmalloc::check_inuse_chunk(mArenaPtr ap, mChunkPtr cp) {
    mChunkPtr next;

    check_chunk(ap, cp);
    if (cp->is_mmapped())
        return; /* mmapped chunks have no next/prev */

    /* Check whether it claims to be in use ... */
    assert(cp->inuse());

    next = cp->nxt_chunk();

    /*
        ... and is surrounded by OK chunks.
        Since more things can be checked with free chunks than inuse ones,
        if an inuse chunk borders them and debug is on, it's worth doing them.
    */
    if (!cp->prev_inuse()) {
        /* Note that we cannot even look at prev unless it is not inuse */
        mChunkPtr pre = cp->pre_chunk();
        assert(pre->nxt_chunk() == cp);
        check_free_chunk(ap, pre);
    }

    if (next == ap->top_chunk) {
        assert(next->prev_inuse());
        assert(next->get_size() >= MINSIZE);
    } else if (!next->inuse())
        check_free_chunk(ap, next);
}

void gcmalloc::check_remalloced_chunk(mArenaPtr ap, mChunkPtr cp, size_t sz) {

    size_t sz_ = cp->size & ~(PREV_INUSE | NON_MAIN_ARENA);
    if (!cp->is_mmapped()) {
        assert(ap == arena_for_chunk(cp));
        if (cp->is_in_non_main_arena())
            assert(ap != main_arena());
        else
            assert(ap == main_arena());
    }

    check_inuse_chunk(ap, cp);

    /* Legal size ... */
    assert((sz & GCMALLOC_ALIGN_MASK) == 0);
    assert((unsigned long) (sz) >= MINSIZE);
    /* ... and alignment */
    assert(cp->is_aligned());

    /* chunk is less than MINSIZE more than request */
    assert((long) (sz_) - (long) (sz) >= 0);
    assert((long) (sz_) - (long) (sz + MINSIZE) < 0);

}


void gcmalloc::check_malloced_chunk(mArenaPtr ap, mChunkPtr cp, size_t sz) {
    /* same as recycled case ... */
    check_remalloced_chunk(ap, cp, sz);

    /*
      ... plus,  must obey implementation invariant that prev_inuse is
      always true of any allocated chunk; i.e., that each allocated
      chunk borders either a previously allocated and still in-use
      chunk, or the base of its memory arena. This is ensured
      by making all allocations from the the `lowest' part of any found
      chunk.  This does not necessarily hold however for chunks
      recycled via fast bins.
    */

    assert(cp->prev_inuse());
}

void gcmalloc::check_malloc_state(mArenaPtr ap) {
    int i;
    mChunkPtr p;
    mChunkPtr q;
    mBinPtr b;
    unsigned int idx;
    size_t size;
    unsigned long total = 0;
    int max_fast_bin;

    /* internal size_t must be no wider than pointer type */
    assert(sizeof(size_t) <= sizeof(char *));

    /* alignment is a power of 2 */
    assert((GCMALLOC_ALIGNMENT & (GCMALLOC_ALIGNMENT - 1)) == 0);

    /* cannot run remaining checks until fully initialized */
    if (ap->top_chunk == nullptr || ap->top_chunk == ap->initial_top())
        return;

    /* pagesize is a power of 2 */
    assert((gcmp.pagesize & (gcmp.pagesize - 1)) == 0);

    /* A contiguous main_arena is consistent with sbrk_base.  */
    if (ap == main_arena() && ap->contiguous())
        assert((char *) gcmp.sbrk_base + ap->system_mem ==
               (char *) ap->top_chunk + ap->top_chunk->get_size());

    /* properties of fastbins */

    /* max_fast is in allowed range */
    /* We hard code it, so no need to check */
//    assert((DEFAULT_MAX_FAST & ~1) <= req2size(MAX_FAST_SIZE));

    max_fast_bin = (int) ((ap->fast_bins).index(DEFAULT_MAX_FAST));

    for (i = 0; i < NUM_FAST_BINS; ++i) {
        p = (ap->fast_bins)[i];

        /* The following test can only be performed for the main arena.
           While mallopt calls malloc_consolidate to get rid of all fast
           bins (especially those larger than the new maximum) this does
           only happen for the main arena.  Trying to do this for any
           other arena would mean those arenas have to be locked and
           malloc_consolidate be called for them.  This is excessive.  And
           even if this is acceptable to somebody it still cannot solve
           the problem completely since if the arena is locked a
           concurrent malloc call might create a new arena which then
           could use the newly invalid fast bins.  */

        /* all bins past max_fast are empty */
        if (ap == main_arena() && i > max_fast_bin)
            assert(p == nullptr);

        while (p != nullptr) {
            /* each chunk claims to be inuse */
            check_inuse_chunk(ap, p);

            total += p->get_size();

            /* chunk belongs in this bin */
            assert((ap->fast_bins).index(p->get_size()) == i);
            p = p->fd;
        }
    }

    if (total != 0)
        assert(ap->have_fast_chunks());
    else if (!ap->have_fast_chunks())
        assert(total == 0);

    /* check normal bins */
    for (i = 1; i < NUM_BINS; ++i) {
        b = (ap->bins).bin_at(i);

        /* binmap is accurate (except for bin 1 == unsorted_chunks) */
        if (i >= 2) {
            unsigned int binbit = (ap->binmap).get_binmap(i);
            int empty = b->bk == b;
            if (!binbit)
                assert(empty);
            else if (!empty)
                assert(binbit);
        }

        for (p = b->bk; p != b; p = p->bk) {
            /* each chunk claims to be free */
            check_free_chunk(ap, p);
            size = p->get_size();
            total += size;
            if (i >= 2) {
                /* chunk belongs in bin */
                idx = bin_idx(size);
                assert(idx == i);
                /* lists are sorted */
                assert(p->bk == b ||
                       (unsigned long) (p->bk)->get_size() >= (unsigned long) p->get_size());

                if (!in_smallbin_range(size)) {
                    if (p->fd_nextsize != nullptr) {
                        if (p->fd_nextsize == p)
                            assert (p->bk_nextsize == p);
                        else {
                            if (p->fd_nextsize == b->fd)
                                assert (p->get_size() < p->fd_nextsize->get_size());
                            else
                                assert (p->get_size() > p->fd_nextsize->get_size());

                            if (p == b->fd)
                                assert (p->get_size() > p->bk_nextsize->get_size());
                            else
                                assert (p->get_size() < p->bk_nextsize->get_size());
                        }
                    } else
                        assert (p->bk_nextsize == nullptr);
                }
            } else if (!in_smallbin_range(size))
                assert (p->fd_nextsize == nullptr && p->bk_nextsize == nullptr);
            /* chunk is followed by a legal chain of inuse chunks */
            for (q = p->nxt_chunk();
                 (q != ap->top_chunk && q->inuse() &&
                  (unsigned long) (q->get_size() >= MINSIZE));
                 q = q->nxt_chunk())
                check_inuse_chunk(ap, q);
        }
    }

    /* top chunk is OK */
    check_chunk(ap, ap->top_chunk);

    /* sanity checks for statistics */
    assert(gcmp.n_mmaps <= gcmp.max_n_mmaps);

    assert((unsigned long) (ap->system_mem) <=
           (unsigned long) (ap->max_system_mem));

    assert((unsigned long) (gcmp.mmapped_mem) <=
           (unsigned long) (gcmp.max_mmapped_mem));

}

void *gcmalloc::malloc_from_sys(size_t nb, mArenaPtr ap) {
    mChunkPtr old_top;      /* incoming value of ap->top_chunk */
    size_t old_size;        /* its size */
    char *old_end;          /* its end address */

    long size;              /* arg to first sbrk or mmap call */
    char *brk;              /* return value from sbrk */

    long correction;        /* arg to 2nd sbrk call */
    char *snd_brk;          /* 2nd return val */

    size_t front_misalign;  /* unusable bytes at front of new space */
    size_t end_misalign;    /* partial page left at end of new space */
    char *aligned_brk;      /* aligned offset into brk */

    mChunkPtr p;                    /* the allocated/returned chunk */
    mChunkPtr remainder;            /* remainder from allocation */
    unsigned long remainder_size;   /* its size */

    unsigned long sum;            /* for updating stats */

    size_t pagemask = gcmp.pagesize - 1;
    bool tried_mmap = false;

    /*
          If the request size meets the mmap threshold, and
          the system supports mmap, and there are few enough currently
          allocated mmapped regions, try to directly map this request
          rather than expanding top.
    */

    if (nb >= gcmp.mmap_threshold && gcmp.n_mmaps < gcmp.n_mmaps_max) {

        char *mm;             /* return value from mmap call*/

        try_mmap:
        /*
            Round up size to nearest page.
            For mmapped chunks, nb no needs to add ONE SIZE_SZ
            like ptmalloc since we don't reuse chunk.

            Note that 'nb' is normalized chunk size!!!
        */

        size = (long) ((nb + pagemask) & ~pagemask);
        tried_mmap = true;

        /* Don't try if size wraps around 0 */
        if ((unsigned long) (size) >= nb) { /* '=' is necessary */

            mm = (char *) (MMAP(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE));

            if (mm != MAP_FAILED) {

                /*
                    The offset to the start of the mmapped region is stored
                    in the prev_size field of the chunk. This allows us to adjust
                    returned start address to meet alignment requirements here
                    and in memalign(), and still be able to compute proper
                    address argument for later munmap in free() and realloc().
                */

#if 1
                /*
                    For glibc, chunk2mem increases the address by 2*SIZE_SZ and
                    GCMALLOC_ALIGN_MASK is 2*SIZE_SZ-1.  Each mmap'ed area is page
                    aligned and therefore definitely GCMALLOC_ALIGN_MASK-aligned.
                */
                assert (((size_t) mm & GCMALLOC_ALIGN_MASK) == 0);
#endif

                p = (mChunkPtr) mm;
                p->set_head(size | IS_MMAPPED);


                /* update statistics */
                sum = gcmp.mmapped_mem += size;
                if (sum > (unsigned long) (gcmp.max_mmapped_mem))
                    gcmp.max_mmapped_mem = sum;

                check_chunk(ap, p);
                return p->chunk2mem();
            }
        }
    }/* can't use mmap */

    /* Record incoming configuration of top */
    old_top = ap->top_chunk;
    old_size = old_top->get_size();
    old_end = (char *) (offset2Chunk(old_top, old_size));

    brk = snd_brk = (char *) (-1);

    /*
       If not the first time through, we require old_size to be
       at least MINSIZE and to have prev_inuse set.
    */

    assert((old_top == ap->initial_top() && old_size == 0) ||
           ((unsigned long) (old_size) >= MINSIZE &&
            old_top->prev_inuse() &&
            ((unsigned long) old_end & pagemask) == 0));

    /* Precondition: not enough current space to satisfy nb request */
    assert((unsigned long) (old_size) < (unsigned long) (nb + MINSIZE));

    /* Precondition: all fastbins are consolidated */
    assert(!ap->have_fast_chunks());

    if (ap != main_arena()) {

        Heap *old_heap, *heap;
        size_t old_heap_size;

        /* First try to extend the current heap. */
        old_heap = old_top->belonged_heap();
        old_heap_size = old_heap->size;
        if (old_heap->grow(MINSIZE + nb - old_size) == 0) {/* In our version, MINSIZE == 64B */
            /* old_heap->size has update in 'grow()' */
            ap->system_mem += old_heap->size - old_heap_size;
            old_top->set_head((((char *) old_heap + old_heap->size) - (char *) old_top)
                              | PREV_INUSE);
        }
            /*
                we are failed to grow the old heap which related to ap。
                Then we will try create new one。
                There is no need to create arena, just (nb + fencepost + heap) is ok。
            */
        else if ((heap = new_heap(nb + (MINSIZE + sizeof(*heap)), gcmp.top_pad))) {
            /* Use a newly allocated heap.  */
            heap->ar_ptr = ap;
            heap->prev_heap = old_heap;
            ap->system_mem += heap->size;

            /* Set up the new top.  */
            ap->top_chunk = offset2Chunk(heap, sizeof(*heap));
            ap->top_chunk->set_head((heap->size - sizeof(*heap)) | PREV_INUSE);

            /*
                Setup fencepost and free the old top chunk.
                The fencepost takes at least MINSIZE bytes, because it might
                become the top chunk again later.  Note that a footer is set
                up, too, although the chunk is marked in use.

                DO NOT add fencepost to used_bin otherwise it will be recycled by gc.
            */

            old_size -= MINSIZE;
            /* the real size of 2nd fencepost is 4 * SIZE_SZ */
            offset2Chunk(old_top, old_size + 2 * SIZE_SZ)->set_head(0 | PREV_INUSE);
            if (old_size >= MINSIZE) {
                offset2Chunk(old_top, old_size)->set_head((2 * SIZE_SZ) | PREV_INUSE);
                offset2Chunk(old_top, old_size)->set_prev_size((2 * SIZE_SZ));
                old_top->set_head(old_size | PREV_INUSE | NON_MAIN_ARENA);
                _int_free(ap, old_top->chunk2mem());
            } else {
                old_top->set_head((old_size + 2 * SIZE_SZ) | PREV_INUSE);
                old_top->set_prev_size(old_size + 2 * SIZE_SZ);
            }
        } else if (!tried_mmap)
            /* We can at least try to use to mmap memory.  */
            goto try_mmap;

    } else { /* av == main_arena */
        /* Request enough space for nb + pad + overhead */
        size = (long) (nb + gcmp.top_pad + MINSIZE);

        /*
            If contiguous, we can subtract out existing space that we hope to
            combine with new space. We add it back later only if
            we don't actually get contiguous space.
        */
        if (ap->contiguous())
            size -= (long) old_size;

        /*
            Round to a multiple of page size.
            If sbrk is not contiguous, this ensures that we only call it
            with whole-page arguments.  And if sbrk is contiguous and
            this is not first time through, this preserves page-alignment of
            previous calls. Otherwise, we correct to page-align below.
        */

        size = (long) ((size + pagemask) & ~pagemask);

        /*
            Don't try to call sbrk if argument is so big as to appear
            negative. Note that since mmap takes size_t arg, it may succeed
            below even if we cannot call sbrk.
        */
        if (size > 0)
            brk = (char *) (sbrk(size));

        /* sbrk failed or size < 0 */
        if (brk == (char *) (-1)) {
            /*
                If have mmap, try using it as a backup when sbrk fails or
                cannot be used. This is worth doing on systems that have "holes" in
                address space, so sbrk cannot extend to give contiguous space, but
                space is available elsewhere.  Note that we ignore mmap max count
                and threshold limits, since the space will not be used as a
                segregated mmap region.
            */

            /* Cannot merge with old top, so add its size back in */
            if (ap->contiguous())
                size = (long) ((size + old_size + pagemask) & ~pagemask);

            /* If we are relying on mmap as backup, then use larger units */
            if ((unsigned long) (size) < MMAP_AS_SBRK_SIZE)
                size = MMAP_AS_SBRK_SIZE;

            /* Don't try if size wraps around 0 */
            if ((unsigned long) (size) > nb) {

                char *mbrk = (char *) (MMAP(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE));

                if (mbrk != MAP_FAILED) {

                    /* We do not need, and cannot use, another sbrk call to find end */
                    brk = mbrk;
                    snd_brk = brk + size;

                    /*
                       Record that we no longer have a contiguous sbrk region.
                       After the first time mmap is used as backup, we do not
                       ever rely on contiguous space since this could incorrectly
                       bridge regions.
                    */
                    ap->set_non_contiguous();
                }
            }
        }

        /* sbrk ok or mmap ok */
        if (brk != (char *) (-1)) {
            if (gcmp.sbrk_base == nullptr)
                gcmp.sbrk_base = brk;
            ap->system_mem += size;

            /* If sbrk extends previous space, we can likewise extend top size. */
            if (brk == old_end && snd_brk == (char *) (-1))
                old_top->set_head((size + old_size) | PREV_INUSE);
            else if (ap->contiguous() && old_size && brk < old_end) {
                /* Oops!  Someone else killed our space..  Can't touch anything.  */
                assert(0);
            }
                /*
                    Otherwise, make adjustments:

                    * If the first time through or noncontiguous, we need to call sbrk
                    just to find out where the end of memory lies.

                    * We need to ensure that all returned chunks from malloc will meet
                    GCMALLOC_ALIGNMENT

                    * If there was an intervening foreign sbrk, we need to adjust sbrk
                    request size to account for fact that we will not be able to
                    combine new space with existing space in old_top.

                    * Almost all systems internally allocate whole pages at a time, in
                    which case we might as well use the whole last page of request.
                    So we allocate enough more memory to hit a page boundary now,
                    which in turn causes future contiguous calls to page-align.
                */

            else {
                front_misalign = 0;
                end_misalign = 0;
                correction = 0;
                aligned_brk = brk;

                /* handle contiguous cases */
                if (ap->contiguous()) {

                    /* Count foreign sbrk as system_mem.  */
                    if (old_size)
                        ap->system_mem += brk - old_end;

                    /* Guarantee alignment of first new chunk made from this space */
                    front_misalign = (size_t) (brk + 2 * SIZE_SZ) & GCMALLOC_ALIGN_MASK;
                    if (front_misalign > 0) {
                        /*
                            Skip over some bytes to arrive at an aligned position.
                            We don't need to specially mark these wasted front bytes.
                            They will never be accessed anyway because
                            prev_inuse of ap->top_chunk (and any chunk created from its start)
                            is always true after initialization.
                        */
                        correction = (long) (GCMALLOC_ALIGNMENT - front_misalign);
                        aligned_brk += correction;
                    }

                    /*
                        If this isn't adjacent to existing space, then we will not
                        be able to merge with old_top space, so must add to 2nd request.
                    */
                    correction += (long) (old_size);

                    /* Extend the end address to hit a page boundary */
                    end_misalign = (size_t) (brk + size + correction);
                    correction += (long) (((end_misalign + pagemask) & ~pagemask) - end_misalign);

                    assert(correction >= 0);
                    snd_brk = (char *) (sbrk(correction));

                    /*
                        If can't allocate correction, try to at least find out current
                        brk.  It might be enough to proceed without failing.

                        Note that if second sbrk did NOT fail, we assume that space
                        is contiguous with first sbrk. This is a safe assumption unless
                        program is multi-threaded but doesn't use locks and a foreign
                        sbrk occurred between our first and second calls.
                    */

                    if (snd_brk == (char *) (-1)) {
                        correction = 0;
                        snd_brk = (char *) (sbrk(0));
                    }
                } else { /* handle non-contiguous cases */
                    /* sbrk/mmap must correctly align */
                    assert(((unsigned long) (brk + 2 * SIZE_SZ) & GCMALLOC_ALIGN_MASK) == 0);

                    /* Find out current end of memory */
                    if (snd_brk == (char *) (-1)) {
                        snd_brk = (char *) (sbrk(0));
                    }
                }

                /* Adjust top based on results of second sbrk */
                if (snd_brk != (char *) (-1)) {
                    ap->top_chunk = (mChunkPtr) aligned_brk;
                    ap->top_chunk->set_head((snd_brk - aligned_brk + correction) | PREV_INUSE);
                    ap->system_mem += correction;

                    /*
                        If not the first time through, we either have a
                        gap due to foreign sbrk or a non-contiguous region.  Insert a
                        double fencepost at old_top to prevent consolidation with space
                        we don't own. These fenceposts are artificial chunks that are
                        marked as inuse and are in any case too small to use.  We need
                        two to make sizes and alignments work out.
                    */

                    if (old_size != 0) {
                        /*
                           Shrink old_top to insert fenceposts, keeping size a
                           multiple of MALLOC_ALIGNMENT. We know there is at least
                           enough space in old_top to do this.
                        */
                        old_size = (old_size - 4 * SIZE_SZ) & ~GCMALLOC_ALIGN_MASK;
                        old_top->set_head(old_size | PREV_INUSE);

                        /*
                            Note that the following assignments completely overwrite
                            old_top when old_size was previously MINSIZE.  This is
                            intentional. We need the fencepost, even if old_top otherwise gets
                            lost.
                        */
                        offset2Chunk(old_top, old_size)->size = (2 * SIZE_SZ) | PREV_INUSE;
                        offset2Chunk(old_top, old_size + 2 * SIZE_SZ)->size = (2 * SIZE_SZ) | PREV_INUSE;

                        /* If possible, release the rest. */
                        if (old_size >= MINSIZE) {
                            _int_free(ap, old_top->chunk2mem());
                        }

                    }
                }
            }
        }
    } /* if (av !=  main_arena()) */

    if ((unsigned long) ap->system_mem > (unsigned long) (ap->max_system_mem))
        ap->max_system_mem = ap->system_mem;
    check_malloc_state(ap);

    /* finally, do the allocation */
    p = ap->top_chunk;
    size = (long) (p->get_size());

    /* check that one of the above allocation paths succeeded */
    if ((unsigned long) (size) >= (unsigned long) (nb + MINSIZE)) {
        remainder_size = size - nb;
        remainder = offset2Chunk(p, nb);
        ap->top_chunk = remainder;
        p->set_head(nb | PREV_INUSE | (ap != main_arena() ? NON_MAIN_ARENA : 0));
        remainder->set_head(remainder_size | PREV_INUSE);
        check_malloced_chunk(ap, p, nb);
        return p->chunk2mem();
    }

    /* catch all failure paths */
    MALLOC_FAILURE_ACTION;
    return nullptr;
}

int gcmalloc::trim_to_sys(size_t pad, mArenaPtr ap) {
    long top_size;          /* Amount of top-most memory */
    long extra;             /* Amount to release */
    long released;          /* Amount actually released */
    char *current_brk;      /* address returned by pre-check sbrk call */
    char *new_brk;          /* address returned by post-check sbrk call */
    size_t pagesz;

    pagesz = gcmp.pagesize;
    top_size = (long) (ap->top_chunk->get_size());

    /* Release in pagesize units, keeping at least one page */
    extra = (long) (((top_size - pad - MINSIZE + (pagesz - 1)) / pagesz - 1) * pagesz);

    if (extra > 0) {

        /*
            Only proceed if end of memory is where we last set it.
            This avoids problems if there were foreign sbrk calls.
        */
        current_brk = (char *) (sbrk(0));
        if (current_brk == (char *) (ap->top_chunk) + top_size) {

            /*
                Attempt to release memory. We ignore sbrk return value,
                and instead call again to find out where new end of memory is.
                This avoids problems if first call releases less than we asked,
                of if failure somehow altered brk value. (We could still
                encounter problems if it altered brk in some very bad way,
                but the only thing we can do is adjust anyway, which will cause
                some downstream failure.)
            */

            sbrk(-extra);
            new_brk = (char *) (sbrk(0));

            if (new_brk != (char *) -1) {
                released = (long) (current_brk - new_brk);

                if (released != 0) {
                    /* Success. Adjust top. */
                    ap->system_mem -= released;
                    ap->top_chunk->set_head((top_size - released) | PREV_INUSE);
                    check_malloc_state(ap);
                    return 1;
                }
            }
        }
    }
    return 0;
}

void gcmalloc::munmap_chunk(mChunkPtr p) {
    size_t size = p->get_size();
    assert (p->is_mmapped());

    /* since we are usually aligned, prev_size is always 0 */
    uintptr_t block = (uintptr_t) p - p->prev_size;
    size_t total_size = p->prev_size + size;

    /*
        Unfortunately we have to do the compilers job by hand here.  Normally
        we would test BLOCK and TOTAL-SIZE separately for compliance with the
        page size.  But gcc does not recognize the optimization possibility
        (in the moment at least) so we combine the two values into one before
        the bit test.
    */
    if (__builtin_expect(((block | total_size) & (gcmp.pagesize - 1)) != 0, 0)) {
        malloc_printerr(check_action, "munmap_chunk(): invalid pointer",
                        p->chunk2mem());
        return;
    }

    gcmp.n_mmaps--;
    gcmp.mmapped_mem -= total_size;
    int ret __attribute__ ((unused)) = munmap((char *) block, total_size);

    /* munmap returns non-zero on failure */
    assert(ret == 0);
}

void *gcmalloc::malloc(size_t bytes) {
    mArenaPtr ar_ptr;
    void *victim;

    /* this isn't a effective way, but I have no tolerance to do more*/
    collect();

    mallocFuncType hook = malloc_hook;
    if (hook != nullptr)
        return (hook)(bytes);
    ar_ptr = get_arena(bytes);

    if (!ar_ptr)
        return nullptr;
    victim = _int_malloc(ar_ptr, bytes);
    if (!victim) {
        /* Maybe the failure is due to running out of mmapped areas. */
        if (ar_ptr != main_arena()) {
            ar_ptr->unlock_this_arena();
            ar_ptr = main_arena();
            ar_ptr->lock_this_arena();
            victim = _int_malloc(ar_ptr, bytes);
            ar_ptr->unlock_this_arena();
        } else {/* ... or sbrk() has failed and there is still a chance to mmap() */
            ar_ptr = get_arena2(ar_ptr->next ? ar_ptr : 0, bytes);
            main_arena()->unlock_this_arena();
            if (ar_ptr) {
                victim = _int_malloc(ar_ptr, bytes);
                main_arena()->unlock_this_arena();
            }
        }
    } else
        ar_ptr->unlock_this_arena();

    /* add to used bin*/
    if(victim)
        used_bin.insert(offset2Chunk(victim, -2 * SIZE_SZ));

    assert(!victim || (mem2Chunk(victim)->is_mmapped()) ||
           ar_ptr == arena_for_chunk(mem2Chunk(victim)));
    return victim;
}

void gcmalloc::free(void *mem) {
    mArenaPtr ar_ptr;
    mChunkPtr p;   /* chunk corresponding to mem */

    freeFuncType hook = free_hook;
    if (hook != nullptr) {
        (hook)(mem);
        return;
    }

    if (mem == nullptr)  /* free(0) has no effect */
        return;

    p = mem2Chunk(mem);

    if (p->is_mmapped())   /* release mmapped memory. */
    {
        /* see if the dynamic brk/mmap threshold needs adjusting */
        if (!gcmp.no_dyn_threshold
            && p->size > gcmp.mmap_threshold
            && p->size <= DEFAULT_MMAP_THRESHOLD_MAX) {

            gcmp.mmap_threshold = p->get_size();
            gcmp.trim_threshold = 2 * gcmp.mmap_threshold;
        }
        munmap_chunk(p);
        return;
    }

    ar_ptr = arena_for_chunk(p);
    ar_ptr->lock_this_arena();
    _int_free(ar_ptr, mem);
    ar_ptr->unlock_this_arena();
}


void *gcmalloc::_int_malloc(mArenaPtr ap, size_t bytes) {
    size_t nb;                      /* normalized request size */
    unsigned int idx;               /* associated bin index */
    mBinPtr bin;                    /* associated bin */
    mFastbinPtr *fb;                /* associated fastbin */

    mChunkPtr victim;               /* inspected/selected chunk */
    size_t size;                    /* its size */
    int victim_index;               /* its bin index */

    mChunkPtr remainder;            /* remainder from a split */
    unsigned long remainder_size;   /* its size */

    unsigned int block;             /* bit map traverser */
    unsigned int bit;               /* bit map traverser */
    unsigned int map;               /* current word of binmap */

    mChunkPtr fwd;                  /* misc temp for linking */
    mChunkPtr bck;                  /* misc temp for linking */

    /*
        Convert request size to internal form by adding SIZE_SZ bytes
        overhead plus possibly more to obtain necessary alignment and/or
        to obtain a size of at least MINSIZE, the smallest allocatable
        size. Also, checked_request2size traps (returning 0) request sizes
        that are so large that they wrap around zero when padded and
        aligned.
    */

    nb = checked_req2size(bytes);

    /*
        If the size qualifies as a fast bin, first check corresponding bin.
        This code is safe to execute even if av is not yet initialized, so we
        can try it without checking, which saves some time on this fast path.
    */

    if (nb <= (unsigned long) (DEFAULT_MAX_FAST)) {
        idx = ap->fast_bins.index(nb);
        fb = &((ap->fast_bins)[idx]);
        if ((victim = *fb) != nullptr) {
            if (__builtin_expect(ap->fast_bins.index(victim->get_size()) != idx, 0))
                malloc_printerr(check_action, "malloc(): memory corruption (fast)",
                                victim->chunk2mem());

            *fb = victim->fd;

            check_remalloced_chunk(ap, victim, nb);
            void *p = victim->chunk2mem();

            if (__builtin_expect(perturb_byte, 0))
                alloc_perturb(p, bytes);
            return p;
        }
    }

    /*
        If a small request, check regular bin.  Since these "smallbins"
        hold one size each, no searching within bins is necessary.
        (For a large request, we need to wait until unsorted chunks are
        processed to find best fit. But for small ones, fits are exact
        anyway, so we can check now, which is faster.)
    */

    if (in_smallbin_range(nb)) {
        idx = smallbin_idx(nb);
        bin = (ap->bins).bin_at(idx);

        if ((victim = bin->bk) != bin) {
            if (victim == nullptr) /* initialization check */
                malloc_consolidate(ap);
            else {
                bck = victim->bk;
                victim->set_inuse();
                bin->bk = bck;
                bck->fd = bin;

                if (ap != main_arena())
                    victim->size |= NON_MAIN_ARENA;
                check_malloced_chunk(ap, victim, nb);
                void *p = victim->chunk2mem();
                if (__builtin_expect(perturb_byte, 0))
                    alloc_perturb(p, bytes);
                return p;
            }
        }
    }

        /*
            If this is a large request, consolidate fastbins before continuing.
            While it might look excessive to kill all fastbins before
            even seeing if there is space available, this avoids
            fragmentation problems normally associated with fastbins.
            Also, in practice, programs tend to have runs of either small or
            large requests, but less often mixtures, so consolidation is not
            invoked all that often in most programs. And the programs that
            it is called frequently in otherwise tend to fragment.
        */

    else {
        idx = largebin_idx(nb);
        if (ap->have_fast_chunks())
            malloc_consolidate(ap);
    }

    /*
        Process recently freed or remaindered chunks, taking one only if
        it is exact fit, or, if this a small request, the chunk is remainder from
        the most recent non-exact fit.  Place other traversed chunks in
        bins.  Note that this step is the only place in any routine where
        chunks are placed in bins.

        The outer loop here is needed because we might not realize until
        near the end of malloc that we should have consolidated, so must
        do so and retry. This happens at most once, and only when we would
        otherwise need to expand memory to service a "small" request.
    */
    for (;;) {
        int iters = 0;
        while ((victim = ap->unsorted_chunks()->bk) != ap->unsorted_chunks()) {
            bck = victim->bk;
            if (__builtin_expect(victim->size <= 2 * SIZE_SZ, 0)
                || __builtin_expect(victim->size > ap->system_mem, 0))
                malloc_printerr(check_action, "malloc(): memory corruption",
                                victim->chunk2mem());

            size = victim->get_size();

            /*
                If a small request, try to use last remainder if it is the
                only chunk in unsorted bin.  This helps promote locality for
                runs of consecutive small requests. This is the only
                exception to best-fit, and applies only when there is
                no exact fit for a small chunk.
            */

            if (in_smallbin_range(nb) &&
                bck == ap->unsorted_chunks() &&
                victim == ap->last_remainder &&
                (unsigned long) (size) > (unsigned long) (nb + MINSIZE)) {
                /* split and reattach remainder */
                remainder_size = size - nb;
                remainder = offset2Chunk(victim, nb);

                ap->unsorted_chunks()->bk = ap->unsorted_chunks()->fd = remainder;// unsorted_chunks(av) == bck
                ap->last_remainder = remainder;
                remainder->bk = remainder->fd = ap->unsorted_chunks();
                if (!in_smallbin_range(remainder_size)) {
                    remainder->fd_nextsize = nullptr;
                    remainder->bk_nextsize = nullptr;
                }

                victim->set_head(nb | PREV_INUSE | (ap != main_arena() ? NON_MAIN_ARENA : 0));
                remainder->set_head(remainder_size | PREV_INUSE);
                remainder->set_foot(remainder_size);

                check_malloced_chunk(ap, victim, nb);
                void *p = victim->chunk2mem();
                if (__builtin_expect(perturb_byte, 0))
                    alloc_perturb(p, bytes);
                return p;
            }

            /* remove from unsorted list */
            ap->unsorted_chunks()->bk = bck;
            bck->fd = ap->unsorted_chunks();

            /* Take now instead of binning if exact fit */
            if (size == nb) {
                victim->set_inuse();
                if (ap != main_arena())
                    victim->size |= NON_MAIN_ARENA;
                check_malloced_chunk(ap, victim, nb);
                void *p = victim->chunk2mem();
                if (__builtin_expect(perturb_byte, 0))
                    alloc_perturb(p, bytes);
                return p;
            }

            /* place chunk in bin */
            if (in_smallbin_range(size)) {
                victim_index = (int) (smallbin_idx(size));
                bck = ap->bins.bin_at(victim_index);
                fwd = bck->fd;
            } else {
                victim_index = (int) (largebin_idx(size));
                bck = ap->bins.bin_at(victim_index);
                fwd = bck->fd;

                /* maintain large bins in sorted order */
                if (fwd != bck) {
                    /* Or with inuse bit to speed comparisons */
                    size |= PREV_INUSE;
                    /* if smaller than smallest, bypass loop below */
                    assert((bck->bk->size & NON_MAIN_ARENA) == 0);
                    if ((unsigned long) (size) < (unsigned long) (bck->bk->size)) {
                        fwd = bck;
                        bck = bck->bk;

                        victim->fd_nextsize = fwd->fd;
                        victim->bk_nextsize = fwd->fd->bk_nextsize;
                        fwd->fd->bk_nextsize = victim->bk_nextsize->fd_nextsize = victim;
                    } else {
                        assert((fwd->size & NON_MAIN_ARENA) == 0);

                        while ((unsigned long) size < fwd->size) {
                            fwd = fwd->fd_nextsize;
                            assert((fwd->size & NON_MAIN_ARENA) == 0);
                        }

                        if ((unsigned long) size == (unsigned long) fwd->size)
                            /* Always insert in the second position.  */
                            fwd = fwd->fd;
                        else {
                            victim->fd_nextsize = fwd;
                            victim->bk_nextsize = fwd->bk_nextsize;
                            fwd->bk_nextsize = victim;
                            victim->bk_nextsize->fd_nextsize = victim;
                        }
                        bck = fwd->bk;
                    }
                } else
                    victim->fd_nextsize = victim->bk_nextsize = victim;
            }

            ap->binmap.mark_bin(victim_index);
            victim->bk = bck;
            victim->fd = fwd;
            fwd->bk = victim;
            bck->fd = victim;

#define MAX_ITERS    10000
            if (++iters >= MAX_ITERS)
                break;
        }

        /*
          If a large request, scan through the chunks of current bin in
          sorted order to find smallest that fits.  Use the skip list for this.
        */

        if (!in_smallbin_range(nb)) {
            bin = ap->bins.bin_at(idx);

            /* skip scan if empty or largest chunk is too small */
            if ((victim = bin->fd) != bin &&
                (unsigned long) (victim->size) >= (unsigned long) (nb)) {

                victim = victim->bk_nextsize;

                while (((unsigned long) (size = victim->get_size()) <
                        (unsigned long) (nb)))
                    victim = victim->bk_nextsize;

                /* Avoid removing the first entry for a size so that the skip
                   list does not have to be rerouted.  */
                if (victim != bin->bk && victim->size == victim->fd->size)
                    victim = victim->fd;

                remainder_size = size - nb;
                victim->unlink(bck, fwd);

                /* Exhaust */
                if (remainder_size < MINSIZE) {
                    victim->set_inuse();
                    if (ap != main_arena())
                        victim->size |= NON_MAIN_ARENA;
                }
                    /* Split */
                else {
                    remainder = offset2Chunk(victim, nb);
                    /* We cannot assume the unsorted list is empty and therefore
                       have to perform a complete insert here.  */
                    bck = ap->unsorted_chunks();
                    fwd = bck->fd;
                    remainder->bk = bck;
                    remainder->fd = fwd;
                    bck->fd = remainder;
                    fwd->bk = remainder;
                    if (!in_smallbin_range(remainder_size)) {
                        remainder->fd_nextsize = nullptr;
                        remainder->bk_nextsize = nullptr;
                    }
                    victim->set_head(nb | PREV_INUSE |
                                     (ap != main_arena() ? NON_MAIN_ARENA : 0));
                    remainder->set_head(remainder_size | PREV_INUSE);
                    remainder->set_foot(remainder_size);
                }
                check_malloced_chunk(ap, victim, nb);
                void *p = victim->chunk2mem();
                if (__builtin_expect(perturb_byte, 0))
                    alloc_perturb(p, bytes);
                return p;
            }
        }

        /*
            Search for a chunk by scanning bins, starting with next largest
            bin. This search is strictly by best-fit; i.e., the smallest
            (with ties going to approximately the least recently used) chunk
            that fits is selected.

            The bitmap avoids needing to check that most blocks are nonempty.
            The particular case of skipping all bins during warm-up phases
            when no chunks have been returned yet is faster than it might look.
        */

        ++idx;
        bin = ap->bins.bin_at(idx);
        block = ap->binmap.idx2block(idx);
        map = ap->binmap[block];
        bit = ap->binmap.idx2bit(idx);


        for (;;) {
            /* Skip rest of block if there are no more set bits in this block.  */
            if (bit > map || bit == 0) {
                do {
                    if (++block >= BIN_MAP_SIZE)  /* out of bins */
                        goto use_top;
                } while ((map = ap->binmap[block]) == 0);

                bin = ap->bins.bin_at(block << BIN_MAP_SHIFT);
                bit = 1;
            }

            /* Advance to bin with set bit. There must be one. */
            while ((bit & map) == 0) {
                bin = ap->bins.next_bin(bin);
                bit <<= 1;
                assert(bit != 0);
            }

            /* Inspect the bin. It is likely to be non-empty */
            victim = bin->bk;

            /*  If a false alarm (empty bin), clear the bit. */
            if (victim == bin) {
                ap->binmap[block] = (map &= ~bit); /* Write through */
                bin = ap->bins.next_bin(bin);
                bit <<= 1;
            } else {
                size = victim->get_size();

                /*  We know the first chunk in this bin is big enough to use. */
                assert((unsigned long) (size) >= (unsigned long) (nb));

                remainder_size = size - nb;

                /* unlink */
                victim->unlink(bck, fwd);

                /* Exhaust */
                if (remainder_size < MINSIZE) {
                    victim->set_inuse();
                    if (ap != main_arena())
                        victim->size |= NON_MAIN_ARENA;
                }
                    /* Split */
                else {
                    remainder = offset2Chunk(victim, nb);

                    /* We cannot assume the unsorted list is empty and therefore
                       have to perform a complete insert here.  */
                    bck = ap->unsorted_chunks();
                    fwd = bck->fd;
                    remainder->bk = bck;
                    remainder->fd = fwd;
                    bck->fd = remainder;
                    fwd->bk = remainder;

                    /* advertise as last remainder */
                    if (in_smallbin_range(nb))
                        ap->last_remainder = remainder;
                    if (!in_smallbin_range(remainder_size)) {
                        remainder->fd_nextsize = nullptr;
                        remainder->bk_nextsize = nullptr;
                    }
                    victim->set_head(nb | PREV_INUSE |
                                     (ap != main_arena() ? NON_MAIN_ARENA : 0));
                    remainder->set_head(remainder_size | PREV_INUSE);
                    remainder->set_foot(remainder_size);
                }
                check_malloced_chunk(ap, victim, nb);
                void *p = victim->chunk2mem();
                if (__builtin_expect(perturb_byte, 0))
                    alloc_perturb(p, bytes);
                return p;
            }
        }

        use_top:
        /*
            If large enough, split off the chunk bordering the end of memory
            (held in ap->top_chunk). Note that this is in accord with the best-fit
            search rule.  In effect, ap->top_chunk is treated as larger (and thus
            less well fitting) than any other available chunk since it can
            be extended to be as large as necessary (up to system
            limitations).

            We require that ap->top_chunk always exists (i.e., has size >=
            MINSIZE) after initialization, so if it would otherwise be
            exhausted by current request, it is replenished. (The main
            reason for ensuring it exists is that we may need MINSIZE space
            to put in fenceposts in sysmalloc.)
        */

        victim = ap->top_chunk;
        size = victim->get_size();

        if ((unsigned long) (size) >= (unsigned long) (nb + MINSIZE)) {
            remainder_size = size - nb;
            remainder = offset2Chunk(victim, nb);
            ap->top_chunk = remainder;
            victim->set_head(nb | PREV_INUSE |
                             (ap != main_arena() ? NON_MAIN_ARENA : 0));
            remainder->set_head(remainder_size | PREV_INUSE);

            check_malloced_chunk(ap, victim, nb);
            void *p = victim->chunk2mem();
            if (__builtin_expect(perturb_byte, 0))
                alloc_perturb(p, bytes);
            return p;
        }

            /*
              If there is space available in fastbins, consolidate and retry,
              to possibly avoid expanding memory. This can occur only if nb is
              in smallbin range so we didn't consolidate upon entry.
            */

        else if (ap->have_fast_chunks()) {
            assert(in_smallbin_range(nb));
            malloc_consolidate(ap);
            idx = smallbin_idx(nb); /* restore original bin index */
        }
            /* Otherwise, relay to handle system-dependent cases */
        else {
            void *p = malloc_from_sys(nb, ap);
            if (p != nullptr && __builtin_expect(perturb_byte, 0))
                alloc_perturb(p, bytes);
            return p;
        }
    }
}

void gcmalloc::_int_free(mArenaPtr ap, void *mem) {
    mChunkPtr p;            /* chunk corresponding to mem */
    size_t size;            /* its size */
    mFastbinPtr *fb;        /* associated fastbin */
    mChunkPtr nextchunk;    /* next contiguous chunk */
    size_t nextsize;        /* its size */
    int nextinuse;          /* true if nextchunk is used */
    size_t prevsize;        /* size of previous contiguous chunk */
    mChunkPtr bck;          /* misc temp for linking */
    mChunkPtr fwd;          /* misc temp for linking */

    const char *errstr = nullptr;

    p = mem2Chunk(mem);
    size = p->get_size();

    /*
        Little security check which won't hurt performance: the
        allocator never wrapps around at the end of the address space.
        Therefore we can exclude some size values which might appear
        here by accident or by "design" from some intruder.
    */
    if (__builtin_expect((uintptr_t) p > (uintptr_t) -size, 0)
        || __builtin_expect(!p->is_aligned(), 0)) {
        errstr = "free(): invalid pointer";
        errout:
        malloc_printerr(check_action, errstr, mem);
        return;
    }
    /* We know that each chunk is at least MINSIZE bytes in size.  */
    if (__builtin_expect(size < MINSIZE, 0)) {
        errstr = "free(): invalid size";
        goto errout;
    }

    check_inuse_chunk(ap, p);

    /*
        If eligible, place chunk on a fastbin so it can be found
        and used quickly in malloc.
    */
    if ((unsigned long) (size) <= (unsigned long) (DEFAULT_MAX_FAST)) {
        if (__builtin_expect(offset2Chunk(p, size)->size <= 2 * SIZE_SZ, 0)
            || __builtin_expect(offset2Chunk(p, size)->get_size()
                                >= ap->system_mem, 0)) {
            errstr = "free(): invalid next size (fast)";
            goto errout;
        }

        ap->set_fast_chunks();
        fb = &(ap->fast_bins[ap->fast_bins.index(size)]);
        /* Another simple check: make sure the top of the bin is not the
           record we are going to add (i.e., double free).  */
        if (__builtin_expect(*fb == p, 0)) {
            errstr = "double free or corruption (fasttop)";
            goto errout;
        }

        if (__builtin_expect(perturb_byte, 0))
            free_perturb(mem, size - SIZE_SZ);

        p->fd = *fb;
        *fb = p;
    }
        /* Consolidate other non-mmapped chunks as they arrive. */
    else if (!p->is_mmapped()) {
        nextchunk = offset2Chunk(p, size);

        /* Lightweight tests: check whether the block is already the
           top block.  */
        if (__builtin_expect(p == ap->top_chunk, 0)) {
            errstr = "double free or corruption (top)";
            goto errout;
        }
        /* Or whether the next chunk is beyond the boundaries of the arena.  */
        if (__builtin_expect(ap->contiguous()
                             && (char *) nextchunk
                                >= ((char *) ap->top_chunk + ap->top_chunk->get_size()), 0)) {
            errstr = "double free or corruption (out)";
            goto errout;
        }
        /* Or whether the block is actually not marked used.  */
        if (__builtin_expect(!nextchunk->prev_inuse(), 0)) {
            errstr = "double free or corruption (!prev)";
            goto errout;
        }

        nextsize = nextchunk->get_size();
        if (__builtin_expect(nextchunk->size <= 2 * SIZE_SZ, 0)
            || __builtin_expect(nextsize >= ap->system_mem, 0)) {
            errstr = "free(): invalid next size (normal)";
            goto errout;
        }

        if (__builtin_expect(perturb_byte, 0))
            free_perturb(mem, size - SIZE_SZ);

        /* consolidate backward */
        if (!p->prev_inuse()) {
            prevsize = p->prev_size;
            size += prevsize;
            p = offset2Chunk(p, -((long) prevsize));
            p->unlink(bck, fwd);
        }

        if (nextchunk != ap->top_chunk) {
            /* get and clear inuse bit */
            nextinuse = nextchunk->inuse();

            /* consolidate forward */
            if (!nextinuse) {
                nextchunk->unlink(bck, fwd);
                size += nextsize;
            } else
                p->clear_inuse();

            /*
                Place the chunk in unsorted chunk list. Chunks are
                not placed into regular bins until after they have
                been given one chance to be used in malloc.
            */

            bck = ap->unsorted_chunks();
            fwd = bck->fd;
            p->fd = fwd;
            p->bk = bck;
            if (!in_smallbin_range(size)) {
                p->fd_nextsize = nullptr;
                p->bk_nextsize = nullptr;
            }
            bck->fd = p;
            fwd->bk = p;

            p->set_head(size | PREV_INUSE);
            p->set_foot(size);

            check_free_chunk(ap, p);
        }
            /*
                If the chunk borders the current high end of memory,
                consolidate into top
            */

        else {
            size += nextsize;
            p->set_head(size | PREV_INUSE);
            ap->top_chunk = p;
            check_chunk(ap, p);
        }

        /*
            If freeing a large space, consolidate possibly-surrounding
            chunks. Then, if the total unused topmost memory exceeds trim
            threshold, ask malloc_trim to reduce top.

            Unless max_fast is 0, we don't know if there are fastbins
            bordering top, so we cannot tell for sure whether threshold
            has been reached unless fastbins are consolidated.  But we
            don't want to consolidate on each free.  As a compromise,
            consolidation is performed if FASTBIN_CONSOLIDATION_THRESHOLD
            is reached.
        */

        if ((unsigned long) (size) >= FAST_BIN_CONSOLIDATION_THRESHOLD) {
            if (ap->have_fast_chunks())
                malloc_consolidate(ap);

            if (ap == main_arena()) { /* temprary annotated*/
                if ((unsigned long) (ap->top_chunk->get_size()) >=
                    (unsigned long) (gcmp.trim_threshold))
//                    trim_to_sys(gcmp.top_pad, ap);
                    ;
            } else { // 非主分配区始终用heap_trim来收缩内存
                /* Always try heap_trim(), even if the top chunk is not
                   large, because the corresponding heap might go away.  */
                Heap *heap = ap->top_chunk->belonged_heap();

                assert(heap->ar_ptr == ap);
                ap->heap_trim(heap, gcmp.top_pad);
            }
        }

    }
        /*
            If the chunk was allocated via mmap, release via munmap(). Note
            that if HAVE_MMAP is false but chunk_is_mmapped is true, then
            user must have overwritten memory. There's nothing we can do to
            catch this error unless MALLOC_DEBUG is set, in which case
            check_inuse_chunk (above) will have triggered error.
        */
    else {
        munmap_chunk(p);
    }
}

void gcmalloc::malloc_consolidate(mArenaPtr ap) {
    mFastbinPtr *fb;                /* current fastbin being consolidated */
    mFastbinPtr *maxfb;             /* last fastbin (for loop control) */
    mChunkPtr p;                    /* current chunk being consolidated */
    mChunkPtr nextp;                /* next chunk to consolidate */
    mChunkPtr unsorted_bin;         /* bin header */
    mChunkPtr first_unsorted;       /* chunk to link to */

    /* These have same use as in free() */
    mChunkPtr nextchunk;
    size_t size;
    size_t nextsize;
    size_t prevsize;
    int nextinuse;
    mChunkPtr bck;
    mChunkPtr fwd;

    /*
      If max_fast is 0, we know that av hasn't
      yet been initialized, in which case do so below
    */

    if (DEFAULT_MAX_FAST != 0) { /* this is always true */
        ap->clear_fast_chunks();

        unsorted_bin = ap->unsorted_chunks();

        /*
            Remove each chunk from fast bin and consolidate it, placing it
            then in unsorted bin. Among other reasons for doing this,
            placing in unsorted bin avoids needing to calculate actual bins
            until malloc is sure that chunks aren't immediately going to be
            reused anyway.
        */


        maxfb = &(ap->fast_bins[NUM_FAST_BINS - 1]);
        fb = &(ap->fast_bins[0]);
        do {
            if ((p = *fb) != nullptr) {
                *fb = nullptr;

                do {
                    check_inuse_chunk(ap, p);
                    nextp = p->fd;
                    /* Slightly streamlined version of consolidation code in free() */

                    size = p->size & ~(PREV_INUSE | NON_MAIN_ARENA);// 这里可以也用next_chunk宏代替吧？？？
                    nextchunk = offset2Chunk(p, size);
                    nextsize = nextchunk->get_size();

                    if (!p->prev_inuse()) {
                        prevsize = p->prev_size;
                        size += prevsize;
                        p = offset2Chunk(p, -((long) prevsize));
                        p->unlink(bck, fwd);
                    }


                    if (nextchunk != ap->top_chunk) { // 下一个chunk 不是top chunk，将合并后的chunk插入到unsorted bin中
                        nextinuse = nextchunk->inuse();

                        if (!nextinuse) {
                            size += nextsize;
                            nextchunk->unlink(bck, fwd);
                        } else
                            p->clear_inuse();

                        first_unsorted = unsorted_bin->fd;
                        unsorted_bin->fd = p;
                        first_unsorted->bk = p;

                        if (!in_smallbin_range(size)) {
                            p->fd_nextsize = nullptr;
                            p->bk_nextsize = nullptr;
                        }

                        p->set_head(size | PREV_INUSE);
                        p->bk = unsorted_bin;
                        p->fd = first_unsorted;
                        p->set_foot(size);
                    } else {
                        size += nextsize;
                        p->set_head(size | PREV_INUSE);
                        ap->top_chunk = p;
                    }

                } while ((p = nextp) != nullptr);

            }
        } while (fb++ != maxfb);
    } else {
        ap->init(false);
        check_malloc_state(ap);
    }
}

void gcmalloc::alloc_perturb(void *p, size_t n) {
    memset(p, (perturb_byte ^ 0xff) & 0xff, n);
}

void gcmalloc::free_perturb(void *p, size_t n) {
    memset(p, perturb_byte & 0xff, n);
}


void gcmalloc::malloc_printerr(int action, const char *str, void *ptr) {
    printf("%s", str);
    if (!ptr)
        printf("%p", ptr);
    abort();
}

/*
     * Scan a region of memory and mark any items in the used list appropriately.
     * Both arguments should be GCMALLOC_ALGIMENT aligned.
     * We will scan the BSS, the used chunks and the stack.
*/

void gcmalloc::scan_region(uintptr_t beg, uintptr_t end) {
    uintptr_t p = beg;  /* travel beg to end */
    mChunkPtr cp;       /* travel used_bin */

    while (p <= end) {
        cp = used_bin.head()->get_next_allocated();
        while (cp != used_bin.tail()) {
            /* Could be traced */
            if ((char *) p >= (char*)cp->chunk2mem() && (char *) p < (char *) (cp->nxt_chunk())) {
                cp->set_marked();
                break;
            }
            cp = cp->get_next_allocated();
        }
        ++p;
    }
}

/*
    WARNNING: there is a bug.
    We must supposed to the memory is contiguous now.
    This should be improved in the future.
    Scan the marked blocks for references to other unmarked blocks.

 */
void gcmalloc::scan_heap() {
    uintptr_t hp;
    mChunkPtr cp, p;

    /* O(n^3) */
    /* maintain a BST could be better */
    for (cp = used_bin.head()->get_next_allocated(); cp != used_bin.tail(); cp = cp->get_next_allocated()) {
        for (p = cp->get_next_allocated(); p != used_bin.tail(); p = p->get_next_allocated()) {
            scan_region((uintptr_t)cp->chunk2mem(),(uintptr_t)(cp->this_chunk() + cp->get_valid_size()));
        }
    }
}

void gcmalloc::mark() {
    scan_heap();
}

void gcmalloc::sweep() {
    mChunkPtr pre = used_bin.head();
    mChunkPtr p = pre->get_next_allocated();

    for ( ; p && p != used_bin.tail(); pre = p, p = p->get_next_allocated()) {
        /* The chunk hasn't been marked. Thus, it must be set free. */
        if (!p->is_marked()) {
            used_bin.remove(p);
            free(p->chunk2mem());
        }
    }

}

void gcmalloc::collect() {
    mark();
    sweep();
}




