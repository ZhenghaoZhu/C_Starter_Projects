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

    // void *y = sf_malloc(500);
    // void *a = sf_malloc(500);
    // void *b = sf_malloc(500);
    // sf_free(a);
    void *x = sf_malloc(16384 - 16 - (sizeof(sf_header) + sizeof(sf_footer)));
    // void *x = sf_malloc(PAGE_SZ << 16);
    // y += 0;
    // b += 0;
    // a += 0;
    x += 0;
    sf_show_heap();

    

    return EXIT_SUCCESS;
}
