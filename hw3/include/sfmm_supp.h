#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


void sf_init_lists();
void sf_flush_quick_list(int listIdx);
void sf_init_first_page();
void sf_put_in_free_list(sf_block *splitBlock, size_t blockSize);
void sf_set_block_footer(sf_block *curBlock);