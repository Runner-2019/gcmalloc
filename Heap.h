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
#ifndef GCMALLOC_HEAP_H
#define GCMALLOC_HEAP_H
#include "config.h"
#include "Chunk.h"


/* create new heap space */
/* Create a new heap. Size is automatically rounded up to a multiple of the page size */
mHeapPtr new_heap(size_t sz, size_t top_pad);

/* heap */
struct Heap {
public:

    /* Delete a heap */
    ~Heap();

    /* Grow a heap. Size is automatically rounded up to a multiple of the page size */
    int grow(size_t sz);

    /* Shrink a heap.  */
    int shrink(size_t sz);

//    Heap(size_t sz, size_t top_pad) {
//        this_heap = create(sz, top_pad);
//    }

public:
    mArenaPtr ar_ptr{};      /* arena for this heap */
//    mHeapPtr this_heap;      /* this heap */
    mHeapPtr prev_heap{};    /* Previous heap. */
    size_t size;             /* Current size in bytes.*/
    size_t protect_size;     /* Size in bytes that has been mprotected PROT_READ|PROT_WRITE.*/
    char m_pad[0];           /* Pad to align*/

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




#endif //GCMALLOC_HEAP_H
