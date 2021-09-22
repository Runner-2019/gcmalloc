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
#include "Bin.h"

/* addressing -- note that bin_at(0) does not exist */
mBinPtr Bins::bin_at(size_t i) {
    return (mBinPtr) (((char *) &(bins[((i) - 1) * 2])) - 2 * SIZE_SZ);
}

/* analog of ++bin */
mBinPtr Bins::next_bin(mBinPtr bin) {
    return ((mBinPtr) ((char *) bin + (sizeof(mChunkPtr) << 1)));
}

mChunkPtr Bins::chunk_at_first(mBinPtr bin) {
    return bin->fd;
}

mChunkPtr Bins::chunk_at_last(mBinPtr bin) {
    return bin->bk;
}


size_t FastBin::index(size_t sz) {
    return (sz >> 3) - 2;
}

size_t BinMap::idx2block(size_t i) {
    return i >> BINMAPSHIFT;
}

size_t BinMap::idx2bit(size_t i) {
    return ((1U << (i & (1U << BINMAPSHIFT)-1)));
}

void BinMap::mark_bin(size_t i) {
    binmap[idx2block(i)] |=  idx2bit(i);
}


void BinMap::unmark_bin(size_t i) {
    binmap[idx2block(i)] &=  ~idx2bit(i);
}


unsigned int BinMap::get_binmap(size_t i) {
    return binmap[idx2block(i)] &  idx2bit(i);
}


