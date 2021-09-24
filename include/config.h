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
#ifndef GCMALLOC_CONFIG_H
#define GCMALLOC_CONFIG_H
#include <cstddef>

#define GCMALLOC_DEBUG

// only support 64-bit
const int SIZE_SZ = sizeof(size_t); // 8B
const int GCMALLOC_ALIGNMENT = 2 * SIZE_SZ; // 16B
const int GCMALLOC_ALIGN_MASK = GCMALLOC_ALIGNMENT - 1;


/* Compile-time constants.  */
const int HEAP_MIN_SIZE = 32 * 1024;
const int HEAP_MAX_SIZE = 1024 * 1024;

/* Chunk */
const int CHUNK_MIN_SIZE = 7 * SIZE_SZ; /* 56B */
const int MINSIZE = (CHUNK_MIN_SIZE + GCMALLOC_ALIGN_MASK) & ~(GCMALLOC_ALIGN_MASK); /* 64B */


/* Bins */
const int DEFAULT_MAX_FAST = 80;
const int NUM_FAST_BINS = 10;
const int FAST_BIN_CONSOLIDATION_THRESHOLD = 65536UL;

const int NUM_SMALL_BINS = 64;
const int NUM_BINS = 128;
const int SMALL_BIN_WIDTH = GCMALLOC_ALIGNMENT;
const int SMALL_BINS_MAX_SIZE = NUM_SMALL_BINS * SMALL_BIN_WIDTH;
const int HAVE_FAST_CHUNKS_BIT = 0x1;
const int NON_CONTIGUOUS_BIT = 0x2;
const int BIN_MAP_SHIFT = 5;
const int BITS_PER_MAP = 1U << BIN_MAP_SHIFT;
const int BIN_MAP_SIZE = NUM_BINS / BITS_PER_MAP;

/*
                        gcmalloc parameters
    Symbol            param #   default    allowed param values
    M_MXFAST          1         64         0-80  (0 disables fastbins)
    M_TRIM_THRESHOLD -1         128*1024   any   (-1U disables trimming)
    M_TOP_PAD        -2         0          any
    M_MMAP_THRESHOLD -3         128*1024   any   (or 0 if no MMAP support)
    M_MMAP_MAX       -4         65536      any   (0 disables use of mmap)
 */
const int DEFAULT_TOP_PAD = 0;
const int DEFAULT_MMAP_MAX = 65536;
const int DEFAULT_MMAP_THRESHOLD = 128 * 1024;
const int DEFAULT_MMAP_THRESHOLD_MAX = 128 * 1024;
const int DEFAULT_TRIM_THRESHOLD = 128 * 1024;
const int DEFAULT_PAGESIZE = 4096; // 32-bit is 4096B

/* mmap */
const int MMAP_AS_SBRK_SIZE = 1024 * 1024;

/* Debug */
const  int check_action = 3;


#endif //GCMALLOC_CONFIG_H
