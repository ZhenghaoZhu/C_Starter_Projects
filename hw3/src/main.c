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

    void *x = sf_malloc(sizeof(double) * 8);
    sf_show_heap();
	void *y = sf_realloc(x, sizeof(int));


    sf_show_heap();
    x += 0;
    y += 0;

    

    return EXIT_SUCCESS;
}
