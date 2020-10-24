#include <stdio.h>
#include "sfmm.h"
#include "sfmm_supp.h"
#include "debug.h"

/*
 * Define WEAK_MAGIC during compilation to use MAGIC of 0x0 for debugging purposes.
 * Note that this feature will be disabled during grading.
 */
#ifdef WEAK_MAGIC
int sf_weak_magic = 1;
#endif

int main(int argc, char const *argv[]) {

    void *u = sf_malloc(200);
    debug("u : %p \n", u);
    /* void *v = */ sf_malloc(300);
    void *w = sf_malloc(200);
    debug("w : %p \n", w);
    /* void *x = */ sf_malloc(500);
    void *y = sf_malloc(200);
    debug("y : %p \n", y);
    /* void *z = */ sf_malloc(700);

    sf_free(y);
    sf_free(w);
    sf_free(u);

    //assert_quick_list_block_count(0, 0);
    //assert_free_block_count(0, 4);
    //assert_free_block_count(208, 3);
    //assert_free_block_count(1904, 1);
    //assert_free_list_size(3, 3);
    //assert_free_list_size(6, 1);

    sf_show_heap();
    // First block in list should be the most recently freed block.
    int i = 3;
    sf_block *bp = sf_free_list_heads[i].body.links.next;
    printf("bp : %p \n", bp);
    printf("mem start: %p \n", sf_mem_start());
    printf("this: %p \n", (char *)u - 2*sizeof(sf_header));
    

    return EXIT_SUCCESS;
}
