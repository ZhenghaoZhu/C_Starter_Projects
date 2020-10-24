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

    void *x = sf_malloc(sizeof(int));
	/* void *y = */ sf_malloc(10);
	debug("1 \n");
    sf_show_heap();
	x = sf_realloc(x, sizeof(int) * 20);
	debug("2 \n");

    sf_show_heap();
    

    return EXIT_SUCCESS;
}
