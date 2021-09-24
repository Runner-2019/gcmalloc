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
using namespace  std;
char *Heap::aligned_heap = nullptr;


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
    if (Heap::aligned_heap) {
        p2 = (char *) (MMAP(Heap::aligned_heap, HEAP_MAX_SIZE,
                            PROT_NONE, MAP_PRIVATE | MAP_NORESERVE));
        Heap::aligned_heap = nullptr;
        if (p2 != MAP_FAILED && ((unsigned long) p2 & (HEAP_MAX_SIZE - 1))) { // not aligned
            munmap(p2, HEAP_MAX_SIZE);
            p2 = (char *) (MAP_FAILED);
        }
    }
    if (p2 == MAP_FAILED) {
        p1 = (char *) MMAP(nullptr, HEAP_MAX_SIZE << 1, PROT_NONE,
                           MAP_PRIVATE | MAP_NORESERVE);
        /*
            if mapped 2 * HEAP_MAX_SIZE success, we only use [p2, p2 + HEAP_MAX_SIZE],
            the left size will return to OS.Don't forget to modify Heap::aligned_heap.
         */

        if (p1 != MAP_FAILED) {
            p2 = (char *) (((unsigned long) p1 + (HEAP_MAX_SIZE - 1))
                           & ~(HEAP_MAX_SIZE - 1));
            ul = p2 - p1;
            if (ul) // p2 == p1
                munmap(p1, ul);
            else
                Heap::aligned_heap = p2 + HEAP_MAX_SIZE;
            munmap(p2 + HEAP_MAX_SIZE, HEAP_MAX_SIZE - ul);
        } else {
            /*
                Try to take the chance that an allocation of
                only HEAP_MAX_SIZE is already aligned.
            */
            p2 = (char *) MMAP(nullptr, HEAP_MAX_SIZE, PROT_NONE,
                               MAP_PRIVATE | MAP_NORESERVE);
            if (p2 == MAP_FAILED)
                return nullptr;
            if ((unsigned long) p2 & (HEAP_MAX_SIZE - 1)) { // not aligned
                munmap(p2, HEAP_MAX_SIZE);
                return nullptr;
            }
        }
    }

    /* Now, we already have HEAP_MAX_SIZE's size. Then we change it's promotion */
    if (mprotect(p2, sz, PROT_READ | PROT_WRITE) != 0) {
        munmap(p2, HEAP_MAX_SIZE);
        return nullptr;
    }

    /* update the size fields of Heap */
    heap = (mHeapPtr)p2;
    heap->set_size(sz);
    heap->set_prot_size(sz);
    return heap;
}

int Heap::grow(size_t sz) {
    size_t page_mask = DEFAULT_PAGESIZE - 1;
    size_t new_size;
    size_t needed_prot_size;

    sz = sz + page_mask & ~page_mask;
    new_size = size + sz;

    /* more than the max size, cannot grow */
    if ( new_size >  HEAP_MAX_SIZE)
        return -1;

    /* need more size */
    if ( new_size > protect_size) {
        needed_prot_size = new_size - protect_size;
        if (mprotect(this_heap() + protect_size, needed_prot_size, PROT_READ | PROT_WRITE) != 0)
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

    /*
        Try to re-map the extra heap space freshly to save memory, and
        make it inaccessible.
        Note that madvise system call is also recommended.
    */
    if ((char *) MMAP(this_heap() + new_size, sz, PROT_NONE,
                      MAP_PRIVATE | MAP_FIXED) == (char *) MAP_FAILED)
        return -2;
    protect_size = new_size;
    size = new_size;
    return 0;
}

Heap::~Heap() {
    if ((char *) (this) + HEAP_MAX_SIZE == Heap::aligned_heap)
        Heap::aligned_heap = nullptr;
    munmap((char *) (this), HEAP_MAX_SIZE);
}

void Heap::set_size(size_t sz) {
    size = sz;
}

void Heap::set_prot_size(size_t sz) {
    protect_size = sz;
}

size_t Heap::get_size() const {
    return size;
}

size_t Heap::get_prot_size() const {
    return protect_size;
}

char *Heap::this_heap() const {
    return (char *) (this);
}


