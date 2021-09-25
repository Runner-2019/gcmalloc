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
#include "include/Bin.h"

/* addressing -- note that bin_at(0) does not exist */
mBinPtr Bins::bin_at(size_t i) {
    return (mBinPtr) (((char *) &(bins[((i) - 1) * 2])) - 2 * SIZE_SZ);
}

/* analog of ++bin */
mBinPtr Bins::next_bin(mBinPtr bin) {
    return ((mBinPtr) ((char *) bin + 16));
}

mChunkPtr Bins::chunk_at_first(mBinPtr bin) {
    return bin->fd;
}

mChunkPtr Bins::chunk_at_last(mBinPtr bin) {
    return bin->bk;
}


size_t FastBins::index(size_t sz) {
    return (sz >> 3) - 2;
}

size_t BinMap::idx2block(size_t i) {
    return i >> BIN_MAP_SHIFT;
}

size_t BinMap::idx2bit(size_t i) {
    return ((1U << (i & (1U << BIN_MAP_SHIFT) - 1)));
}

void BinMap::mark_bin(size_t i) {
    binmap[idx2block(i)] |= idx2bit(i);
}


void BinMap::unmark_bin(size_t i) {
    binmap[idx2block(i)] &= ~idx2bit(i);
}


unsigned int BinMap::get_binmap(size_t i) {
    return binmap[idx2block(i)] & idx2bit(i);
}

/* always insert in the head of list */
void UsedBin::insert(mChunkPtr p) {
    auto pre = tail()->get_prev_allocated();
    p->set_next_allocated(tail());
    p->set_prev_allocated(pre);
    pre->set_next_allocated(p);
    tail()->set_prev_allocated(p);
}

void UsedBin::remove(mChunkPtr p) {
    auto pre = p->get_prev_allocated();
    auto nxt = p->get_next_allocated();
    pre->set_next_allocated(nxt);
    nxt->set_prev_allocated(pre);
}

mChunkPtr UsedBin::head() {
    auto head = (mChunkPtr)m_data;
    head->set_head(MINSIZE);
    return (mChunkPtr)m_data;
}

mChunkPtr UsedBin::tail() {
    auto tail = (mChunkPtr)(m_data + MINSIZE);
    tail->set_head(MINSIZE);
    return tail;
}


