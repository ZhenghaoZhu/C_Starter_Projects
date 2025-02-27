#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"
#define TEST_TIMEOUT 15
/*
 * Define WEAK_MAGIC during compilation to use MAGIC of 0x0 for debugging purposes.
 * Note that this feature will be disabled during grading.
 */
// #ifdef WEAK_MAGIC
// int sf_weak_magic = 1;
// #endif

/*
 * Assert the total number of free blocks of a specified size.
 * If size == 0, then assert the total number of all free blocks.
 */
void assert_free_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_FREE_LISTS; i++) {
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	while(bp != &sf_free_list_heads[i]) {
	    if(size == 0 || size == ((bp->header ^ MAGIC) & ~0xf))
		cnt++;
	    bp = bp->body.links.next;
	}
    }
    if(size == 0) {
	cr_assert_eq(cnt, count, "Wrong number of free blocks (exp=%d, found=%d)",
		     count, cnt);
    } else {
	cr_assert_eq(cnt, count, "Wrong number of free blocks of size %ld (exp=%d, found=%d)",
		     size, count, cnt);
    }
}

/*
 * Assert the total number of quick list blocks of a specified size.
 * If size == 0, then assert the total number of all quick list blocks.
 */
void assert_quick_list_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_QUICK_LISTS; i++) {
	sf_block *bp = sf_quick_lists[i].first;
	while(bp != NULL) {
	    if(size == 0 || size == ((bp->header ^ MAGIC) & ~0xf))
		cnt++;
	    bp = bp->body.links.next;
	}
    }
    if(size == 0) {
	cr_assert_eq(cnt, count, "Wrong number of quick list blocks (exp=%d, found=%d)",
		     count, cnt);
    } else {
	cr_assert_eq(cnt, count, "Wrong number of quick list blocks of size %ld (exp=%d, found=%d)",
		     size, count, cnt);
    }
}

/*
 * Assert that the free list with a specified index has the specified number of
 * blocks in it.
 */
void assert_free_list_size(int index, int size) {
    int cnt = 0;
    sf_block *bp = sf_free_list_heads[index].body.links.next;
    while(bp != &sf_free_list_heads[index]) {
	cnt++;
	bp = bp->body.links.next;
    }
    cr_assert_eq(cnt, size, "Free list %d has wrong number of free blocks (exp=%d, found=%d)",
		 index, size, cnt);
}

/*
 * Assert that the quick list with a specified index has the specified number of
 * blocks in it.
 */
void assert_quick_list_size(int index, int size) {
    int cnt = 0;
    sf_block *bp = sf_quick_lists[index].first;
    while(bp != NULL) {
	cnt++;
	bp = bp->body.links.next;
    }
    cr_assert_eq(cnt, size, "Quick list %d has wrong number of free blocks (exp=%d, found=%d)",
		 index, size, cnt);
}

Test(sfmm_basecode_suite, malloc_an_int, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	int *x = sf_malloc(sizeof(int));

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(4048, 1);
	assert_free_list_size(7, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + PAGE_SZ == sf_mem_end(), "Allocated more than necessary!");
}

Test(sfmm_basecode_suite, malloc_four_pages, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	// We want to allocate up to exactly four pages.
	void *x = sf_malloc(16384 - 16 - (sizeof(sf_header) + sizeof(sf_footer)));

	cr_assert_not_null(x, "x is NULL!");
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sfmm_basecode_suite, malloc_too_large, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(PAGE_SZ << 16);

	cr_assert_null(x, "x is not NULL!");
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(65520, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sfmm_basecode_suite, free_quick, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	/* void *x = */ sf_malloc(8);
	void *y = sf_malloc(32);
	/* void *z = */ sf_malloc(1);

	sf_free(y);

	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(48, 1);
	assert_free_block_count(0, 1);
	assert_free_block_count(3968, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, free_no_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	/* void *x = */ sf_malloc(8);
	void *y = sf_malloc(200);
	/* void *z = */ sf_malloc(1);

	sf_free(y);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 2);
	assert_free_block_count(208, 1);
	assert_free_block_count(3808, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, free_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	/* void *w = */ sf_malloc(8);
	void *x = sf_malloc(200);
	void *y = sf_malloc(300);
	/* void *z = */ sf_malloc(4);

	sf_free(y);
	sf_free(x);


	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 2);
	assert_free_block_count(528, 1);
	assert_free_block_count(3488, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, freelist, .timeout = TEST_TIMEOUT) {
	void *u = sf_malloc(200);
	/* void *v = */ sf_malloc(300);
	void *w = sf_malloc(200);
	/* void *x = */ sf_malloc(500);
	void *y = sf_malloc(200);
	/* void *z = */ sf_malloc(700);
	sf_free(y);
	sf_free(w);
	sf_free(u);


	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 4);
	assert_free_block_count(208, 3);
	assert_free_block_count(1904, 1);
	assert_free_list_size(3, 3);
	assert_free_list_size(6, 1);

	// First block in list should be the most recently freed block.
	int i = 3;
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	cr_assert_eq(bp, (char *)u - 2*sizeof(sf_header),
		     "Wrong first block in free list %d: (found=%p, exp=%p)",
                     i, bp, (char *)u - 2*sizeof(sf_header));
}

Test(sfmm_basecode_suite, realloc_larger_block, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(int));
	/* void *y = */ sf_malloc(10);
	x = sf_realloc(x, sizeof(int) * 20);

	sf_block *bp = (sf_block *)((char *)x - 2*sizeof(sf_header));
	cr_assert((bp->header ^ MAGIC) & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert(((bp->header ^ MAGIC) & ~0xf) == 96, "Realloc'ed block size not what was expected!");
	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(32, 1);
	assert_free_block_count(0, 1);
	assert_free_block_count(3920, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_splinter, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(int) * 20);
	void *y = sf_realloc(x, sizeof(int) * 16);

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_block *bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
	cr_assert((bp->header ^ MAGIC) & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert(((bp->header ^ MAGIC) & ~0xf) == 96, "Block size not what was expected!");

	// There should be only one free block of size 3984.
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(3984, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_free_block, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(double) * 8);
	void *y = sf_realloc(x, sizeof(int));
	cr_assert_not_null(y, "y is NULL!");

	sf_block *bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
	cr_assert((bp->header ^ MAGIC) & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert(((bp->header ^ MAGIC) & ~0xf) == 32, "Realloc'ed block size not what was expected!");

	// After realloc'ing x, we can return a block of size 32 to the freelist.
	// This block will go into the main freelist and be coalesced.  Note that we don't put split
        // blocks into the quick lists because their sizes are not sizes that were requested by the
	// client, so they are not very likely to satisfy a new request.
	assert_quick_list_block_count(0, 0);	
	assert_free_block_count(0, 1);
	assert_free_block_count(4048, 1);
}

//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

//Test(sfmm_student_suite, student_test_1, .timeout = TEST_TIMEOUT) {
//}

Test(sfmm_student_suite, quicklist_flush_test, .timeout = TEST_TIMEOUT) {

	void *a = sf_malloc(1);
    void *b = sf_malloc(1);
    void *c = sf_malloc(1);
    void *d = sf_malloc(2);
    void *e = sf_malloc(2);
    void *f = sf_malloc(2);

    sf_free(a);
    sf_free(b);
    sf_free(c);
    sf_free(d);
    sf_free(e);
    sf_free(f);

	assert_quick_list_block_count(0, 1); // Should only have one 32 byte block in quicklist, rest are flushed
	assert_quick_list_size(0, 1);
	assert_free_block_count(160, 1); // The coalesced block 
	assert_free_list_size(7,1);
}

Test(sfmm_student_suite, use_quick_list, .timeout = TEST_TIMEOUT){
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

	assert_quick_list_block_count(0, 0);
	for(int i = 0; i < NUM_QUICK_LISTS; i++){
		assert_quick_list_size(i, 0);
	}
    assert_free_block_count(3712, 1);
    assert_free_list_size(7,1);
}

Test(sfmm_student_suite, another_quick_list_test, .timeout = TEST_TIMEOUT){
    void *x = sf_malloc(60);
    void *y = sf_malloc(70);
    void *z = sf_malloc(70);

    sf_free(x);
    sf_free(z);
    sf_free(y);

    assert_quick_list_block_count(80, 3);
	for(int i = 0; i < NUM_QUICK_LISTS; i++){
		if(i == 3){
			assert_quick_list_size(i, 3);
		}
		else {
			assert_quick_list_size(i, 0);
		}
	}
    assert_free_block_count(0, 1);
    assert_free_block_count(3840, 1);
}

Test(sfmm_student_suite, malloc_a_alot, .timeout = TEST_TIMEOUT){
    void *a = sf_malloc(4000);
    void *b = sf_malloc(4000);
    void *c = sf_malloc(4000);
    sf_malloc(4050);
    sf_free(a);
    sf_free(c);
    sf_free(b);

    assert_free_block_count(12048, 1);
	assert_free_list_size(9,1);
    assert_quick_list_block_count(0, 0);
    assert_quick_list_size(5,0);
}

Test(sfmm_student_suite, malloc_and_malloc_too_much, .timeout = TEST_TIMEOUT){
    void *x = sf_malloc(110);
    void *y = sf_malloc(1050);
    void *z = sf_malloc(2000);
    sf_free(y);
    sf_malloc(PAGE_SZ << 16);
    sf_free(x);
    sf_free(z);

    assert_quick_list_block_count(0,1);
	assert_quick_list_size(6, 1);
    assert_free_list_size(9,1);
    assert_free_block_count(65392, 1);
}
