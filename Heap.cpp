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
#include "include/Heap.h"


mHeapPtr new_heap(size_t sz, size_t top_pad) {
    size_t page_mask = DEFAULT_PAGESIZE - 1;
    char *p1, *p2;
    unsigned long ul;
    mHeapPtr heap;

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