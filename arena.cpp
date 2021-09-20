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
#include "arena.h"

mHeapPtr Heap::create(size_t sz, size_t top_pad) {
    size_t page_mask = DEFAULT_PAGESIZE - 1;
    char *p1, *p2;
    unsigned long ul;

    if (sz + top_pad < HEAP_MIN_SIZE)
        sz = HEAP_MIN_SIZE;
    else if (sz + top_pad <= HEAP_MAX_SIZE)
        sz += top_pad;
    else if (sz > HEAP_MAX_SIZE)
        return nullptr;
    else
        sz = HEAP_MAX_SIZE;

    sz = (sz + page_mask) & ~page_mask;

    /*
        A memory region aligned to a multiple of HEAP_MAX_SIZE is needed.
        No swap space needs to be reserved for the following large
        mapping (on Linux, this is the case for all non-writable mappings
        anyway).
     */
    p2 = (char *) MAP_FAILED;
    if (aligned_heap) {
        p2 = (char *) (MMAP(aligned_heap, HEAP_MAX_SIZE,
                            PROT_NONE, MAP_PRIVATE | MAP_NORESERVE));
        aligned_heap = nullptr;
        if (p2 != MAP_FAILED && ((unsigned long) p2 & (HEAP_MAX_SIZE - 1))) { // not aligned
            munmap(p2, HEAP_MAX_SIZE);
            p2 = (char *) (MAP_FAILED);
        }

        if (p2 == MAP_FAILED) {
            p1 = (char *) MMAP(nullptr, HEAP_MAX_SIZE << 1, PROT_NONE,
                               MAP_PRIVATE | MAP_NORESERVE);
            /*
                if mapped 2 * HEAP_MAX_SIZE success, we only use [p2, p2 + HEAP_MAX_SIZE],
                the left size will return to OS.Don't forget to modify aligned_heap.
             */

            if (p1 != MAP_FAILED) {
                p2 = (char *) (((unsigned long) p1 + (HEAP_MAX_SIZE - 1))
                               & ~(HEAP_MAX_SIZE - 1));
                ul = p2 - p1;
                if (ul) // p2 == p1
                    munmap(p1, ul);
                else
                    aligned_heap = p2 + HEAP_MAX_SIZE;
                munmap(p2 + HEAP_MAX_SIZE, HEAP_MAX_SIZE - ul);
            } else {
                /* Try to take the chance that an allocation of only HEAP_MAX_SIZE
               is already aligned. */
                p2 = (char *) MMAP(nullptr, HEAP_MAX_SIZE, PROT_NONE,
                                   MAP_PRIVATE | MAP_NORESERVE);
                if (p2 == MAP_FAILED)
                    return nullptr;
                if ((unsigned long) p2 & (HEAP_MAX_SIZE - 1)) { // 未对齐
                    munmap(p2, HEAP_MAX_SIZE);
                    return nullptr;
                }
            }
        }

        /* Now, we already have HEAP_MAX_SIZE's size. Then we change it's promotion */
        if (mprotect(p2, size, PROT_READ | PROT_WRITE) != 0) {
            munmap(p2, HEAP_MAX_SIZE);
            return nullptr;
        }

        // update the fields of Heap
        size = sz;
        protect_size = size;
    }
}

int Heap::grow(size_t sz) {
    size_t page_mask = DEFAULT_PAGESIZE - 1;
    size_t new_size;

    sz = sz + page_mask & ~page_mask;
    new_size = size + sz;

    /* more than the max size, cannot grow */
    if ((unsigned long) new_size > (unsigned long) HEAP_MAX_SIZE)
        return -1;

    /* need more size */
    if ((unsigned long) new_size > protect_size) {
        if (mprotect((char *) this + protect_size,
                     (unsigned long) new_size - protect_size,
                     PROT_READ | PROT_WRITE) != 0)
            return -2;
        protect_size = new_size;
    }

    /* size is enough */
    size = new_size;
    return 0;
}

int Heap::shrink(size_t sz) {
    /* If execute ok, the left size in heap */
    size_t new_size = size - sz;

    /* new_size can not hold a heap*/
    if (new_size < (long) sizeof(Heap))
        return -1;

    /* Try to re-map the extra heap space freshly to save memory, and
       make it inaccessible. */
    if ((char *) MMAP((char *) this + new_size, sz, PROT_NONE,
                      MAP_PRIVATE | MAP_FIXED) == (char *) MAP_FAILED)
        return -2;
    protect_size = new_size;
    size = new_size;
    return 0;
}

Heap::~Heap() {
    if ((char *) (this) + HEAP_MAX_SIZE == aligned_heap)
        aligned_heap = nullptr;
    munmap((char *) (this), HEAP_MAX_SIZE);
}


int Arena::heap_trim(Heap *heap, size_t pad) {
    unsigned long pagesz = DEFAULT_PAGESIZE;
    mChunkPtr p, bck = nullptr, fwd = nullptr;
    mHeapPtr prev_heap;
    size_t new_size, top_size, extra;

    /* Can this heap go away completely? */
    while (topChunk == offset2Chunk(heap, sizeof(*heap))) {
        prev_heap = heap->prev_heap;
        /* Acquire the last fencepost chunk */
        p = offset2Chunk(prev_heap, prev_heap->size - (MINSIZE - 2 * SIZE_SZ));
        assert(p->size == (0 | PREV_INUSE)); /* must be fencepost */

        /* Acquire the first fencepost chunk */
        p = p->pre_chunk();

        // the size of two fencepost chunk
        new_size = p->get_size() + (MINSIZE - 2 * SIZE_SZ);
        assert(new_size > 0 && new_size < (long) (2 * MINSIZE));

        if (!p->prev_inuse())
            new_size += p->prev_size;
        assert(new_size > 0 && new_size < HEAP_MAX_SIZE);

        /* new_size equals the freed size */
        if (new_size + (HEAP_MAX_SIZE - prev_heap->size) < pad + MINSIZE + pagesz)
            break;

        /* delete this heap */
        system_mem -= heap->size;
        delete heap;
        heap = prev_heap;
        /* set new top_chunk */
        if (!p->prev_inuse()) { /* consolidate backward */
            p = p->pre_chunk();
            unlink(p, bck, fwd);
        }
        assert(((unsigned long) ((char *) p + new_size) & (pagesz - 1)) == 0);
        assert(((char *) p + new_size) == ((char *) heap + heap->size));
        topChunk = p;
        topChunk->set_head(new_size | PREV_INUSE);
        /*check_chunk(ar_ptr, top_chunk);*/
    }

    /*
      首先查看 top chunk 的大小，如果 top chunk 的大小减去 pad 和 MINSIZE 小于一页大小，
      返回退出，否则调用 shrink_heap()函数对当前 sub_heap 进行收缩，将空闲的整数个页收缩
      掉，仅剩下不足一页的空闲内存，如果 shrink_heap()失败，返回退出，否则，更新内存使用
      统计，更新 top chunk 的大小。
     */

    top_size = topChunk->get_size();
    extra = ((top_size - pad - MINSIZE + (pagesz - 1)) / pagesz - 1) * pagesz;
    if (extra < (long) pagesz)
        return 0;

    /* Try to shrink. */
    if (heap->shrink(extra) != 0)
        return 0;

    system_mem -= extra;

    /* Success. Adjust top accordingly. */
    topChunk->set_head((top_size - extra) | PREV_INUSE);
    /*check_chunk(ar_ptr, top_chunk);*/
    return 1;
}


void Arena::unlink(mChunkPtr p, mChunkPtr bk, mChunkPtr fd) {
    /* deal with small bins */
    fd = p->fd;
    bk = p->bk;
    assert(fd->bk == p && bk->fd == p);
    fd->bk = bk;
    bk->fd = fd;

    /* deal with large bin */
    if (!(p->in_smallbin_range()) && p->fd_nextsize != nullptr) {
        assert(p->fd_nextsize->bk_nextsize == p);
        assert(p->bk_nextsize->fd_nextsize == p);
        if (fd->fd_nextsize == nullptr) {
            if (p->fd_nextsize == p)
                fd->fd_nextsize = fd->bk_nextsize = fd;
            else {
                fd->fd_nextsize = p->fd_nextsize;
                fd->bk_nextsize = p->bk_nextsize;
                p->fd_nextsize->bk_nextsize = fd;
                p->bk_nextsize->fd_nextsize = fd;
            }
        } else {
            p->fd_nextsize->bk_nextsize = fd->bk_nextsize;
            p->bk_nextsize->fd_nextsize = fd->fd_nextsize;
        }
    }
}

void Arena::init(bool is_main_arena) {
    int i;
    mbinPtr bin;

    /* Establish circular links for normal bins */
    for (i = 1; i < NBINS; ++i) {
        bin = bin_at(i);
        bin->fd = bin->bk = bin;
    }

    if (!is_main_arena)
        set_non_contiguous();
    else
//        set_max_fast(DEFAULT_MXFAST);
        ;

    flags |= HAVE_FASTCHUNKS_BIT;

    initial_top();
}


mbinPtr Arena::bin_at(size_t i) {
    return (mbinPtr) (((char *) &(bins[((i) - 1) * 2])) - 2 * SIZE_SZ);
}

void Arena::initial_top() {
    topChunk = bin_at(1);
}

void Arena::set_non_contiguous() {
    flags |= NONCONTIGUOUS_BIT;
}


