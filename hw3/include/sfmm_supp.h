#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


void sf_init_lists();
void sf_flush_quick_list(int listIdx);
void sf_init_first_page();
void sf_put_in_free_list(sf_block *splitBlock, size_t blockSize);
void sf_put_in_free_list_helper(sf_block *splitBlock, int curIdx);
void sf_set_block_footer(sf_block *curBlock);
void sf_coalesce(void* middleBlock, size_t blockSz, size_t prevFooter, bool makingLargePage);
int sf_put_in_quick_list(sf_block *curBlock, size_t blockSize);
void sf_put_in_quick_list_helper(sf_block *curBlock, int curIdx);
size_t sf_take_out_last_three_bits_and_get_block_size(sf_block *curBlock);
void* sf_get_past_block(void * middleBlockHeader, size_t blockSz);
void* sf_get_next_block(void * middleBlockHeader, size_t blockSz);
void sf_remove_from_free_list(sf_block *removedBlock);
size_t sf_get_block_sz(sf_block *curBlock);