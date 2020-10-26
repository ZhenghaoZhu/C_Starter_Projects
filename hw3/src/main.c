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


	void *a = sf_malloc(10);
    void *b = sf_malloc(10);
    void *c = sf_malloc(10);
    void *d = sf_malloc(20);
    void *e = sf_malloc(20);
    void *f = sf_malloc(20);
	void *g = sf_malloc(10);
    void *h = sf_malloc(10);
    void *i = sf_malloc(10);
    void *j = sf_malloc(20);
    void *k = sf_malloc(20);
    sf_malloc(20);

    sf_free(a);
    sf_free(b);
    sf_free(c);
    sf_free(d);
    sf_free(e);
	sf_free(f);
	sf_free(g);
    sf_free(h);
    sf_free(i);
    sf_free(j);
    debug("+++5 \n");
	sf_show_heap();
	debug("+++5 \n");
    sf_free(k);
    debug("+++6 \n");
	sf_show_heap();
	debug("+++6 \n");


	// assert_quick_list_block_count(0, 1); // Should only have one 32 byte block in quicklist, rest are flushed
	// assert_free_block_count(160, 1); // The coalesced block 

    return EXIT_SUCCESS;
}
