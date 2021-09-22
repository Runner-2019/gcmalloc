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
#include "Chunk.h"

char *Chunk::this_chunk() {
    return (char *) (this);
}

mChunkPtr Chunk::nxt_chunk() {
    return offset2Chunk(this_chunk(), get_size());
}

mChunkPtr Chunk::pre_chunk() {
    return (mChunkPtr) (this_chunk() - prev_size);
}

void *Chunk::chunk2mem() {
    return (void *) (this_chunk() + 2 * SIZE_SZ);
}

bool Chunk::is_aligned() {
    return true;
}

bool Chunk::prev_inuse() {
    return size & PREV_INUSE;
}

bool Chunk::is_mmaped() {
    return size & IS_MMAPPED;
}

bool Chunk::is_in_non_main_arena() {
    return size & NON_MAIN_ARENA;
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
    return ((mHeapPtr)((unsigned long)(this_chunk()) & ~(HEAP_MAX_SIZE-1)));
}



















