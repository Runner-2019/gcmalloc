/*
  --------------------------------------
     ╭╩══╮╔══════╗╔══════╗╔══════╗ 
    ╭    ╠╣      ╠╣      ╠╣      ╣
    ╰⊙══⊙╯╚◎════◎╝╚◎════◎╝╚◎════◎╝
  --------------------------------------
  @date:   2021-九月-27
  @author: xiaomingZhang2020@outlook.com
  --------------------------------------
*/
#ifndef GCMALLOC_GCMALLOCPAR_H
#define GCMALLOC_GCMALLOCPAR_H

struct gcmallocPar {
    /* Tunable parameters */
    unsigned long trim_threshold;
    size_t mmap_threshold;
    size_t top_pad;

    /* Memory map  support */
    int n_mmaps;
    int n_mmaps_max;
    int max_n_mmaps;

    /*
        the mmap_threshold is dynamic, until the user sets
        it manually, at which point we need to disable any
        dynamic behavior.
    */
    int no_dyn_threshold;

    /* Cache malloc_getpagesize */
    unsigned int pagesize;

    /* Statistics */
    size_t mmapped_mem;
    size_t max_mmapped_mem;

    /* First address handed out by sbrk.  */
    char *sbrk_base;
};



#endif //GCMALLOC_GCMALLOCPAR_H
