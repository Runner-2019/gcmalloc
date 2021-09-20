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

bool Chunk::is_non_main_arena() {
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

void Chunk::set_footer() {
    nxt_chunk()->prev_size = get_size();
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


bool Chunk::in_smallbin_range() {
    return get_size() < SMALLBIN_MAX_SIZE;
}

size_t Chunk::smallbin_idx() {
    return get_size() >> 4;
}

size_t Chunk::largebin_idx() {
    unsigned long sz = get_size();
    size_t ret = 0;
    if (sz >> 6 <= 48)
        ret += 48 + (sz >> 6);
    if (sz >> 9 <= 20)
        ret += 91 + (sz >> 9);
    if (sz >> 12 <= 10)
        ret += 110 + (sz >> 12);
    if (sz >> 15 <= 4)
        ret += 119 + (sz >> 15);
    if (sz >> 18 <= 2)
        ret += 124 + (sz >> 18);
    else
        ret += 126;
    return ret;
}

size_t Chunk::bin_idx() {
    return in_smallbin_range() ? smallbin_idx() : largebin_idx();
}





















