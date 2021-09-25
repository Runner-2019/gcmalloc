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
#include "Chunk.h"
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

/* Create a new heap. Size is automatically rounded up to a multiple of the page size */
mHeapPtr new_heap(size_t sz, size_t top_pad);


/* heap */
class Heap {
    friend class Arena;
public:
    int grow(size_t sz);
    int shrink(size_t sz);
    [[nodiscard]] char *this_heap() const;
    Heap() : ar_ptr(nullptr), prev_heap(nullptr), size(0), protect_size(0) {}
    ~Heap();

public:
    static char *aligned_heap;
public:
    /* operate data fields */
    void set_size(size_t sz);
    void set_prot_size(size_t sz);
    [[nodiscard]] size_t get_size() const;
    [[nodiscard]] size_t get_prot_size() const;


private:
    mArenaPtr ar_ptr;        /* arena for this heap */
    mHeapPtr prev_heap;      /* Previous heap. */
    size_t size;             /* Current size in bytes.*/
    size_t protect_size;     /* Size in bytes that can be read or write.*/

};


#endif //GCMALLOC_HEAP_H
