/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"
#include "sfmm_supp.h"


// NOTE: Page 886 for example code
void *sf_malloc(size_t size) {

    // Min block size of 32 bytes
    // Check if quicklists has a block large enough for the current requested size
    // If not, look at freelist and iterate over it until you find a large enough block
    // If no blocks are large enough in freelist, use memgrow to add another memory page (more than one call may be required)
    // If the above can't be satisfied with multiple calls to mem_grow, set sf_errno to ENOMEM and return NULL

    if(size == 0){
        return NULL;
    }

    if(sf_mem_start() == sf_mem_end()){
        sf_init_first_page();
    }

    int i = 0;
    size_t blockSize = 0;
    size_t payloadSz = 0;
    size_t paddingSz = 0;

    blockSize = size + 8;
    while(blockSize % 16 != 0){
        blockSize += 1;
        paddingSz += 1;
    }

    payloadSz = size + paddingSz;
    while(blockSize % 16 != 0 || payloadSz < 24){ // Make sure there is enough space for footer if block is freed.
        blockSize += 1;
        payloadSz += 1;
        paddingSz += 1;
    }

    
    // Initialize quicklists with nothing in them
    size_t curListBlockSz = 0;
    for(i = 0; i < NUM_QUICK_LISTS; i++){
        if(sf_quick_lists[i].length > 0){
            curListBlockSz = sf_quick_lists[i].first->header ^ MAGIC;

            if((curListBlockSz & THIS_BLOCK_ALLOCATED) == 0x4){
                curListBlockSz -= 4;
            }

            if((curListBlockSz & PREV_BLOCK_ALLOCATED) == 0x2){
                curListBlockSz -= 2;
            }

            if(curListBlockSz >= blockSize){
                return NULL; // FIXME:
            }
        }
    }

    sf_block *curBlock;
    sf_block *splitBlock;
    for(i = 0; i < NUM_FREE_LISTS; i++){
        curBlock = &sf_free_list_heads[i]; // Sentinel
        while(curBlock->body.links.next != &sf_free_list_heads[i]){
            curBlock = curBlock->body.links.next;
            curListBlockSz = curBlock->header ^ MAGIC;
            if((curListBlockSz & THIS_BLOCK_ALLOCATED) == 0x4){
                curListBlockSz -= 4;
            }

            if((curListBlockSz & PREV_BLOCK_ALLOCATED) == 0x2){
                curListBlockSz -= 2;
            }
            if(curListBlockSz >= blockSize){
                if((curListBlockSz - blockSize) >= 32){
                    splitBlock = (sf_block*)((char*)curBlock + blockSize);
                    splitBlock->header = curListBlockSz - blockSize;
                    sf_set_block_footer(splitBlock);
                    sf_block *newPrev = curBlock->body.links.prev;
                    sf_block *newNext = curBlock->body.links.next;
                    newPrev->body.links.next = newNext;
                    newNext->body.links.prev = newPrev;
                    sf_put_in_free_list(splitBlock, curListBlockSz - blockSize);
                    curBlock->header = blockSize;
                    curBlock->header += 4;
                    curBlock->header ^= MAGIC;
                    debug("Returned: %p \n", curBlock);
                    return curBlock;
                }
                else {
                    // Splinter, just return it
                    return curBlock;
                }
                
                return NULL; // FIXME:
            }
        }
    }
    
    return NULL;
}

// NOTE: Page 885 for example code
void sf_free(void *pp) {
    sf_block *curPtr = (sf_block *) pp;
    size_t curHeader;
    if(curPtr == NULL){
        abort();
    }
    curHeader = curPtr->header;
    curHeader ^= MAGIC;
    // if(curHeader < 32 || curHeader % 16 != 0 || )
    // return;
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    return NULL;
}


void sf_init_lists(){
    int i;
    
    // Initialize quicklists with nothing in them
    for(i = 0; i < NUM_QUICK_LISTS; i++){
        sf_quick_lists[i].length = 0;
        sf_quick_lists[i].first = NULL;
    }

    // Prev and next circle back to sentinel at first
    for(i = 0; i < NUM_FREE_LISTS; i++){
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
    }

    return;
}

void sf_flush_quick_list(int listIdx){
    return;
}

void sf_init_first_page(){
    sf_init_lists();
    sf_block* pgBlock = (sf_block*) sf_mem_grow();
    sf_header pgBlockSz = 4080;
    pgBlockSz ^= MAGIC;
    pgBlock->header = pgBlockSz;
    void* setFooter = pgBlock;
    setFooter += sizeof(char) * 4080;
    *((long int *) setFooter) = (long int) pgBlockSz;

    sf_free_list_heads[6].body.links.next = pgBlock;
    pgBlock->body.links.prev = &sf_free_list_heads[6];
    pgBlock->body.links.next = &sf_free_list_heads[6];
    // sf_show_free_lists();
    // printf("\n");
    // sf_show_quick_lists();
    printf("\n");
    sf_show_heap();
    printf("\nsf_mem_start: %p \n", sf_mem_start());
    printf("sf_mem_end: %p \n", sf_mem_end());
    return;
}

void sf_put_in_free_list(sf_block *splitBlock, size_t blockSize){
    
    if(blockSize <= 32){
        sf_block *oldNext = sf_free_list_heads[0].body.links.next;
        splitBlock->body.links.next = oldNext;
        splitBlock->body.links.prev = &sf_free_list_heads[0];
        sf_free_list_heads[0].body.links.next = splitBlock;
    }
    else if(blockSize <= 64){
        sf_block *oldNext = sf_free_list_heads[1].body.links.next;
        splitBlock->body.links.next = oldNext;
        splitBlock->body.links.prev = &sf_free_list_heads[1];
        sf_free_list_heads[1].body.links.next = splitBlock;
    }
    else if(blockSize <= 128){
        sf_block *oldNext = sf_free_list_heads[2].body.links.next;
        splitBlock->body.links.next = oldNext;
        splitBlock->body.links.prev = &sf_free_list_heads[2];
        sf_free_list_heads[2].body.links.next = splitBlock;
    }
    else if(blockSize <= 256){
        sf_block *oldNext = sf_free_list_heads[3].body.links.next;
        splitBlock->body.links.next = oldNext;
        splitBlock->body.links.prev = &sf_free_list_heads[3];
        sf_free_list_heads[3].body.links.next = splitBlock;
    }
    else if(blockSize <= 512){
        sf_block *oldNext = sf_free_list_heads[4].body.links.next;
        splitBlock->body.links.next = oldNext;
        splitBlock->body.links.prev = &sf_free_list_heads[4];
        sf_free_list_heads[4].body.links.next = splitBlock;
    }
    else if(blockSize <= 1024){
        sf_block *oldNext = sf_free_list_heads[5].body.links.next;
        splitBlock->body.links.next = oldNext;
        splitBlock->body.links.prev = &sf_free_list_heads[5];
        sf_free_list_heads[5].body.links.next = splitBlock;
    }
    else if(blockSize <= 2048){
        sf_block *oldNext = sf_free_list_heads[6].body.links.next;
        splitBlock->body.links.next = oldNext;
        splitBlock->body.links.prev = &sf_free_list_heads[6];
        sf_free_list_heads[6].body.links.next = splitBlock;
    }
    else if(blockSize <= 4096){
        sf_block *oldNext = sf_free_list_heads[7].body.links.next;
        splitBlock->body.links.next = oldNext;
        splitBlock->body.links.prev = &sf_free_list_heads[7];
        sf_free_list_heads[7].body.links.next = splitBlock;
    }
    else if(blockSize <= 8192){
        sf_block *oldNext = sf_free_list_heads[8].body.links.next;
        splitBlock->body.links.next = oldNext;
        splitBlock->body.links.prev = &sf_free_list_heads[8];
        sf_free_list_heads[8].body.links.next = splitBlock;
    }
    else{
        sf_block *oldNext = sf_free_list_heads[9].body.links.next;
        splitBlock->body.links.next = oldNext;
        splitBlock->body.links.prev = &sf_free_list_heads[9];
        sf_free_list_heads[9].body.links.next = splitBlock;
    }

    return;
}

void sf_set_block_footer(sf_block *curBlock){
    void* setFooter = curBlock;
    setFooter += sizeof(char) * curBlock->header;
    *((long int *) setFooter) = (long int) curBlock->header;
    return;
}

