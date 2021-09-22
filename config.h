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
const int CHUNK_MIN_SIZE = 4 * SIZE_SZ; /* 32B */
const int MINSIZE = (CHUNK_MIN_SIZE + GCMALLOC_ALIGN_MASK) & ~(GCMALLOC_ALIGN_MASK);


/* Bins */
const int NBINS = 128;
const int NFASTBINS = 80;
const int NSMALLBINS = 64;
const int SMALLBIN_WIDTH = GCMALLOC_ALIGNMENT;
const int SMALLBIN_MAX_SIZE = NSMALLBINS * SMALLBIN_WIDTH;
const int DEFAULT_MXFAST = 64;
const int HAVE_FASTCHUNKS_BIT = 0x1;
const int NONCONTIGUOUS_BIT = 0x2;
const int FASTBIN_CONSOLIDATION_THRESHOLD = 65536UL;
const int BINMAPSHIFT = 5;
const int BITSPERMAP = 1U << BINMAPSHIFT;
const int BINMAPSIZE = NBINS / BITSPERMAP;

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
#ifndef MAP_FAILED
#define MAP_FAILED (void*)(-1);
#endif
const int MMAP_AS_SBRK_SIZE = 1024 * 1024;

/* Debug */
const  int check_action = 3;


#endif //GCMALLOC_CONFIG_H
