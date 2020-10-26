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


	void *a = sf_malloc(20);
    void *b = sf_malloc(50);
    void *c = sf_malloc(115);
    void *d = sf_malloc(130);

    sf_free(a);
    sf_free(b);
    sf_free(c);
    sf_free(d);

    sf_malloc(1);
    sf_malloc(55);
    sf_malloc(120);
    sf_malloc(135);

	sf_show_heap();


	// assert_quick_list_block_count(0, 1); // Should only have one 32 byte block in quicklist, rest are flushed
	// assert_free_block_count(160, 1); // The coalesced block 

    return EXIT_SUCCESS;
}
