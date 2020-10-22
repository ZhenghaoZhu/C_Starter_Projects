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

    int* ptr = sf_malloc(sizeof(int));
    *ptr = 320320320e-320;
    debug("%p\n", ptr);
    sf_show_heap();
    // sf_free(ptr);

    // sf_malloc(1);
    // sf_malloc(25);
    // sf_malloc(110);

    return EXIT_SUCCESS;
}
