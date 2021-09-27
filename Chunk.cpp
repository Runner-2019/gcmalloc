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
#include "include/Chunk.h"

char *Chunk::this_chunk() const {
    return (char *) (this);
}

mChunkPtr Chunk::nxt_chunk() const {
    return offset2Chunk(this_chunk(), get_size());
}

mChunkPtr Chunk::pre_chunk() const {
    return (mChunkPtr) (this_chunk() - prev_size);
}

void *Chunk::chunk2mem() const {
    return (void *) (this_chunk() + 2 * SIZE_SZ);
}

bool Chunk::is_aligned() const {
    return ((unsigned long) this_chunk() & GCMALLOC_ALIGN_MASK) == 0;
}

bool Chunk::prev_inuse() const {
    return size & PREV_INUSE;
}

bool Chunk::is_mmapped() const {
    return size & IS_MMAPPED;
}

bool Chunk::is_in_non_main_arena() const {
    return size & NON_MAIN_ARENA;
}


bool Chunk::is_marked() const {
    return size & GC_MARKED;
}

size_t Chunk::get_size() const {
    return size & ~allBits;
}

void Chunk::set_head_size(size_t sz) {
    size = size & allBits | sz;
}

void Chunk::set_head(size_t sizeAndBits) {
    size = sizeAndBits;
}

void Chunk::set_foot(size_t sz) {
    nxt_chunk()->prev_size = sz;
}

void Chunk::set_prev_size(size_t sz) {
    prev_size = sz;
}

size_t Chunk::get_prev_size() const {
    return prev_size & ~allBits;
}


size_t Chunk::get_size_field() const {
    return size;
}

size_t Chunk::get_prev_size_field() const {
    return prev_size;
}


void Chunk::set_marked() {
    size |= GC_MARKED;
}

void Chunk::clear_marked() {
    size = size & ~GC_MARKED;
}

/* This chunk's inuse information is stored in next chunk */
bool Chunk::inuse() {
    return nxt_chunk()->size & PREV_INUSE;
}

void Chunk::set_inuse() {
    nxt_chunk()->size |= PREV_INUSE;
}

void Chunk::clear_inuse() {
    nxt_chunk()->size &= ~PREV_INUSE;
}

void Chunk::unlink(mChunkPtr bck, mChunkPtr fwd) {
    /* deal with small bins */
    fwd = fd;
    bck = bk;
    assert(fd->bk == this && bk->fd == this);
    fwd->bk = bk;
    bck->fd = fd;

    /* deal with large bin */
    if (!(in_smallbin_range(get_size())) && fd_nextsize != nullptr) {
        assert(fd_nextsize->bk_nextsize == this);
        assert(bk_nextsize->fd_nextsize == this);
        if (fwd->fd_nextsize == nullptr) {
            if (fd_nextsize == this)
                fwd->fd_nextsize = fwd->bk_nextsize = fwd;
            else {
                fwd->fd_nextsize = fd_nextsize;
                fwd->bk_nextsize = bk_nextsize;
                fd_nextsize->bk_nextsize = fwd;
                bk_nextsize->fd_nextsize = fwd;
            }
        } else {
            fd_nextsize->bk_nextsize = fwd->bk_nextsize;
            bk_nextsize->fd_nextsize = fwd->fd_nextsize;
        }
    }
}

mHeapPtr Chunk::belonged_heap() {
    return ((mHeapPtr) ((unsigned long) (this_chunk()) & ~(HEAP_MAX_SIZE - 1)));
}


/* next & prev field can not allocated */
size_t Chunk::get_valid_size() const {
    return get_size() - 4 * SIZE_SZ;
}

void Chunk::set_next_allocated(mChunkPtr p) {
    auto next = (unsigned long *) ((char *) chunk2mem() + get_valid_size());
    *next = (p ? (unsigned long) p : (unsigned long) 0);
}

mChunkPtr Chunk::get_next_allocated() const {
    return (mChunkPtr) (*(unsigned long *) ((char *) chunk2mem() + get_valid_size()));
}


void Chunk::set_prev_allocated(mChunkPtr p) {
    auto prev = (unsigned long *) ((char *) (chunk2mem()) + get_valid_size() + SIZE_SZ);
    *prev = (p ? (unsigned long) p : (unsigned long) 0);
}

mChunkPtr Chunk::get_prev_allocated() const {
    return (mChunkPtr) (*(unsigned long *) ((char *) (chunk2mem()) + get_valid_size() + SIZE_SZ));
}
























