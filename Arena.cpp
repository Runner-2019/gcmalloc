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
#include "include/Arena.h"

/* some bits ops */
void Arena::set_non_contiguous() {
    flags |= NON_CONTIGUOUS_BIT;
}

void Arena::set_contiguous() {
    flags &= ~NON_CONTIGUOUS_BIT;
}

bool Arena::have_fast_chunks() const{
    return (flags & HAVE_FAST_CHUNKS_BIT) == 0;
}

void Arena::clear_fast_chunks() {
    flags |= HAVE_FAST_CHUNKS_BIT;
}

void Arena::set_fast_chunks() {
    flags &= ~HAVE_FAST_CHUNKS_BIT;
}

bool Arena::contiguous() const {
    return (flags & NON_CONTIGUOUS_BIT) == 0;
}

bool Arena::non_contiguous() const{
    return !contiguous();
}



/* unsorted chunk ops */
mChunkPtr Arena::initial_top() {
    return unsorted_chunks();
}

mChunkPtr Arena::unsorted_chunks() {
    return (mChunkPtr) bins.bin_at(1);
}



void Arena::init(bool is_main_arena) {
    int i;
    mBinPtr bin;

    /* Establish circular links for normal bins */
    for (i = 1; i < NUM_BINS; ++i) {
        bin = bins.bin_at(i);
        bin->fd = bin->bk = bin;
    }

    if (!is_main_arena)
        set_non_contiguous();
    else
        set_contiguous();

    /* only in there wo set the have fast chunks bit */
    flags |= HAVE_FAST_CHUNKS_BIT;

    top_chunk =  initial_top();
//    top_chunk->set_head(0);
    init_this_arena_lock();

}


int Arena::heap_trim(Heap *heap, size_t pad) {
    unsigned long pagesz = DEFAULT_PAGESIZE;
    mChunkPtr p, bck = nullptr, fwd = nullptr;
    mHeapPtr prev_heap;
    size_t new_size, top_size, extra;

    /* Can this heap go away completely? */
    while (top_chunk == offset2Chunk(heap, sizeof(*heap))) {
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

        /* new_size and the freed size */
        if (new_size + (HEAP_MAX_SIZE - prev_heap->size) < pad + MINSIZE + pagesz)
            break;

        /* delete this heap */
        system_mem -= heap->size;
        delete heap;
        heap = prev_heap;

        /* set new top_chunk */
        if (!p->prev_inuse()) { /* consolidate backward */
            p = p->pre_chunk();
            p->unlink(bck, fwd);
        }
        assert(((unsigned long) ((char *) p + new_size) & (pagesz - 1)) == 0);
        assert(((char *) p + new_size) == ((char *) heap + heap->size));
        top_chunk = p;
        top_chunk->set_head(new_size | PREV_INUSE);

    }

    /*
      首先查看 top chunk 的大小，如果 top chunk 的大小减去 pad 和 MINSIZE 小于一页大小，
      返回退出，否则调用 shrink_heap()函数对当前 sub_heap 进行收缩，将空闲的整数个页收缩
      掉，仅剩下不足一页的空闲内存，如果 shrink_heap()失败，返回退出，否则，更新内存使用
      统计，更新 top chunk 的大小。
     */

    top_size = top_chunk->get_size();
    extra = ((top_size - pad - MINSIZE + (pagesz - 1)) / pagesz - 1) * pagesz;
    if (extra < (long) pagesz)
        return 0;

    /* Try to shrink. */
    if (heap->shrink(extra) != 0)
        return 0;

    system_mem -= extra;

    /* Success. Adjust top accordingly. */
    top_chunk->set_head((top_size - extra) | PREV_INUSE);
    /*check_chunk(ar_ptr, top_chunk);*/
    return 1;
}

void Arena::lock_this_arena() {
    m_mutex.lock();
}

void Arena::unlock_this_arena() {
    m_mutex.unlock();
}

bool Arena::trylock_this_arena() {
    return  m_mutex.trylock();
}

void Arena::init_this_arena_lock() {
    m_mutex.init();
}








