/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
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

    if((int)size < 0){
        sf_errno = ENOMEM;
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

    // debug("Block Size : %li, Payload Size: %li, Padding Size: %li \n", blockSize, payloadSz, paddingSz);
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
                return NULL;  // TODO:
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
                    splitBlock->header = ((curListBlockSz - blockSize) | PREV_BLOCK_ALLOCATED) ^ MAGIC;
                    splitBlock->prev_footer = (blockSize | THIS_BLOCK_ALLOCATED) ^ MAGIC;
                    sf_set_block_footer(splitBlock);
                    sf_block *newPrev = curBlock->body.links.prev;
                    sf_block *newNext = curBlock->body.links.next;
                    newPrev->body.links.next = newNext;
                    newNext->body.links.prev = newPrev;
                    sf_put_in_free_list(splitBlock, curListBlockSz - blockSize);
                    curBlock->header = (blockSize | THIS_BLOCK_ALLOCATED | PREV_BLOCK_ALLOCATED) ^ MAGIC;
                    // sf_show_heap();
                    return curBlock->body.payload;
                }
                else {
                    // Creates a splinter, just return it. (First-fit policy)
                    return curBlock->body.payload;
                }
            }
        }
    }

    // sf_block * newPage;
    // if((newPage = sf_mem_grow()) != NULL){
    //     coalesce(lastBlock, newPage)
    //     sf_malloc(size)
    // } else {
    //     sf_errno = ERRMEM
    //     return NULL;
    // } 
    
    return NULL;
}

// NOTE: Page 885 for example code
void sf_free(void *pp) {
    
    sf_block *curBlock = (sf_block*)(pp - sizeof(char) * 16);
    debug("free curBlock ptr : %p \n", curBlock);
    // setFooter += sizeof(char) * (curBlock->header ^ MAGIC);
    bool hadPrevAll = false;
    size_t curBlockSz;
    long int curBlockInt;

    if(curBlock == NULL){
        abort();
    }

    curBlockInt = (long int)pp;
    if(curBlockInt % 16 != 0){
        abort();
    }
    curBlockSz = curBlock->header;
    curBlockSz ^= MAGIC;
    if((curBlockSz & THIS_BLOCK_ALLOCATED) == 0x4){
        curBlockSz -= 4;
    } else {
        abort();
    }

    if((curBlockSz & PREV_BLOCK_ALLOCATED) == 0x2){ 
        curBlockSz -= 2;
        hadPrevAll = true;
    }
    
    if(curBlockSz < 32 || (curBlockSz % 16 != 0) || (curBlockInt < (unsigned long)sf_mem_start()) || (unsigned long)((char*)curBlock + curBlockSz) > (unsigned long)sf_mem_end()){
        abort();
    }

    if((curBlock->header & PREV_BLOCK_ALLOCATED) == 0){
        sf_block *past_block = sf_get_past_block(curBlock, curBlockSz);
        unsigned long newCurBlockInt = (unsigned long) past_block;
        if((newCurBlockInt >= (unsigned long)sf_mem_start()) && ((past_block->header & THIS_BLOCK_ALLOCATED) != 0)){
            abort();
        }
    }

    if (sf_put_in_quick_list(curBlock, curBlockSz) == true){
        return;
    }

    sf_block *next_block = sf_get_next_block(curBlock, curBlockSz);
    next_block->header = (((next_block->header ^ MAGIC) & ~PREV_BLOCK_ALLOCATED) ^ MAGIC);
    sf_set_block_footer(next_block);

    if(hadPrevAll){
        curBlock->header = (curBlockSz | PREV_BLOCK_ALLOCATED) ^ MAGIC;
    }
    else {
        curBlock->header = curBlockSz ^ MAGIC;
    }
    
    sf_set_block_footer(curBlock);

    sf_put_in_free_list(curBlock, curBlockSz);

    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    sf_block *curBlock = (sf_block*)(pp - sizeof(char) * 16);
    // setFooter += sizeof(char) * (curBlock->header ^ MAGIC);
    size_t curBlockSz;
    long int curBlockInt;

    if(curBlock == NULL){
        sf_errno = EINVAL;
        abort();
    }

    curBlockInt = (long int)pp;
    if(curBlockInt % 16 != 0){
        sf_errno = EINVAL;
        abort();
    }
    curBlockSz = curBlock->header;
    
    curBlockSz ^= MAGIC;
    if((curBlockSz & THIS_BLOCK_ALLOCATED) == 0x4){
        curBlockSz -= 4;
    } else {
        sf_errno = EINVAL;
        abort();
    }

    if((curBlockSz & PREV_BLOCK_ALLOCATED) == 0x2){ 
        curBlockSz -= 2;
    }
    
    if(curBlockSz < 32 || (curBlockSz % 16 != 0) || (curBlockInt < (unsigned long)sf_mem_start()) || (unsigned long)((char*)curBlock + curBlockSz) > (unsigned long)sf_mem_end()){
        sf_errno = EINVAL;
        abort();
    }
    
    if((curBlock->header & PREV_BLOCK_ALLOCATED) == 0){
        sf_block *past_block = sf_get_past_block(curBlock, curBlockSz);
        curBlockInt = (long int)past_block;
        if((curBlockInt >= (unsigned long)sf_mem_start()) && ((past_block->header & THIS_BLOCK_ALLOCATED) != 0)){
            sf_errno = EINVAL;
            abort();
        }
    }

    if(rsize == 0){
        sf_free(pp);
        return NULL;
    }

    if(curBlockSz == (rsize + 8)){ // rsize same as current payload plus header
        return pp;
    } 

    if(curBlockSz < rsize){ // TODO: Give bigger block than original
        sf_block * newBiggerBlock = sf_malloc(rsize);
        if(newBiggerBlock == NULL){
            return NULL;
        }
        memcpy(newBiggerBlock, curBlock, curBlockSz - 8);
        debug("TO FREE \n");
        sf_free((sf_block*)((char*)curBlock + 16));
        debug("OUT OF  FREE \n");
        return newBiggerBlock; 
    }
    
    if(curBlockSz > rsize){ // TODO: Give smaller block than original
        size_t difference = rsize + 8;
        while(difference % 16 != 0){
            difference += 1;
        }
        if((curBlockSz - (rsize + (size_t)8)) < 32){ // Splinter don't split
            return pp;
        }
        else { // TODO: Coalesce for this
            sf_block *splitBlock = NULL;
            size_t newBlockSz = rsize + 8;
            while(newBlockSz % 16 != 0 || newBlockSz <= 16){
                newBlockSz += 1;
            }
            splitBlock = (sf_block*)((char*)curBlock + newBlockSz);
            splitBlock->header = ((curBlockSz - newBlockSz) | PREV_BLOCK_ALLOCATED) ^ MAGIC;
            splitBlock->prev_footer = (newBlockSz | THIS_BLOCK_ALLOCATED) ^ MAGIC;
            sf_set_block_footer(splitBlock);
            sf_block *newPrev = curBlock->body.links.prev;
            sf_block *newNext = curBlock->body.links.next;
            newPrev->body.links.next = newNext;
            newNext->body.links.prev = newPrev;
            sf_put_in_free_list(splitBlock, curBlockSz - newBlockSz);
            curBlock->header = (newBlockSz | THIS_BLOCK_ALLOCATED | PREV_BLOCK_ALLOCATED) ^ MAGIC;
            return curBlock->body.payload;
        }
    }

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

    sf_free_list_heads[7].body.links.next = pgBlock;
    pgBlock->body.links.prev = &sf_free_list_heads[7];
    pgBlock->body.links.next = &sf_free_list_heads[7];

    // sf_show_heap();
    return;
}

void sf_put_in_free_list(sf_block *splitBlock, size_t blockSize){
    
    if(blockSize <= 32){
        sf_put_in_free_list_helper(splitBlock, 0);
    }
    else if(blockSize <= 64){
        sf_put_in_free_list_helper(splitBlock, 1);
    }
    else if(blockSize <= 128){
        sf_put_in_free_list_helper(splitBlock, 2);
    }
    else if(blockSize <= 256){
        sf_put_in_free_list_helper(splitBlock, 3);
    }
    else if(blockSize <= 512){
        sf_put_in_free_list_helper(splitBlock, 4);
    }
    else if(blockSize <= 1024){
        sf_put_in_free_list_helper(splitBlock, 5);
    }
    else if(blockSize <= 2048){
        sf_put_in_free_list_helper(splitBlock, 6);
    }
    else if(blockSize <= 4096){
        sf_put_in_free_list_helper(splitBlock, 7);
    }
    else if(blockSize <= 8192){
        sf_put_in_free_list_helper(splitBlock, 8);
    }
    else{
        sf_put_in_free_list_helper(splitBlock, 9);
    }
    return;
}

void sf_put_in_free_list_helper(sf_block *splitBlock, int curIdx){
    sf_block *oldNext = sf_free_list_heads[curIdx].body.links.next;
    oldNext->body.links.prev = splitBlock;
    splitBlock->body.links.next = oldNext;
    splitBlock->body.links.prev = &sf_free_list_heads[curIdx];
    sf_free_list_heads[curIdx].body.links.next = splitBlock;
    return;
}

void sf_set_block_footer(sf_block *curBlock){
    size_t offset = (curBlock->header ^ MAGIC) & ~0x7;
    void* setFooter = curBlock;
    setFooter += sizeof(char) * offset;
    *((long int *) setFooter) = (long int) curBlock->header;
    return;
}

void sf_coalesce(void* middleBlock, size_t blockSz){
    return;
}

int sf_put_in_quick_list(sf_block *curBlock, size_t blockSize){
    if(blockSize == 32){
        sf_put_in_quick_list_helper(curBlock, 0);
        return true;
    }
    else if(blockSize == 48){
        sf_put_in_quick_list_helper(curBlock, 1);
        return true;
    }
    else if(blockSize == 64){
        sf_put_in_quick_list_helper(curBlock, 2);
        return true;
    }
    else if(blockSize == 80){
        sf_put_in_quick_list_helper(curBlock, 3);
        return true;
    }
    else if(blockSize == 96){
        sf_put_in_quick_list_helper(curBlock, 4);
        return true;
    }
    else if(blockSize == 112){
        sf_put_in_quick_list_helper(curBlock, 5);
        return true;
    }
    else if(blockSize == 128){
        sf_put_in_quick_list_helper(curBlock, 6);
        return true;
    }
    else if(blockSize == 144){
        sf_put_in_quick_list_helper(curBlock, 7);
        return true;
    }
    else if(blockSize == 160){
        sf_put_in_quick_list_helper(curBlock, 8);
        return true;
    }
    else if(blockSize == 176){
        sf_put_in_quick_list_helper(curBlock, 9);
        return true;
    }
    else {
        return false;
    }
}

void sf_put_in_quick_list_helper(sf_block *curBlock, int curIdx){
    if(sf_quick_lists[curIdx].length == 5){
        sf_flush_quick_list(curIdx); // Should reset length here
    }
    if(sf_quick_lists[curIdx].first != NULL){
        sf_quick_lists[curIdx].length += 1; // Add to quicklist
        sf_block *oldFirst = sf_quick_lists[curIdx].first;
        curBlock->body.links.next = oldFirst;
        // curBlock->body.links.prev = NULL;
        // oldFirst->body.links.prev = curBlock;
        sf_quick_lists[curIdx].first = curBlock;
        return;
    } else {
        sf_quick_lists[curIdx].length += 1;
        // curBlock->body.links.prev = NULL;
        curBlock->body.links.next = NULL;
        sf_quick_lists[curIdx].first = curBlock;
        return;
    }
}

size_t sf_take_out_last_three_bits_and_get_block_size(sf_block *curBlock){
    size_t curBlockSz = curBlock->header ^ MAGIC;
    if((curBlockSz & THIS_BLOCK_ALLOCATED) == 0x4){
        curBlockSz -= 4;
    }

    if((curBlockSz & PREV_BLOCK_ALLOCATED) == 0x2){ 
        curBlockSz -= 2;
    }
    curBlock->header = curBlockSz ^ MAGIC;
    
    return curBlockSz;
}

void* sf_get_past_block(void * middleBlockHeader, size_t blockSz){
    sf_block *pastBlock = middleBlockHeader;
    pastBlock = (sf_block*)((char*)middleBlockHeader - blockSz);
    return pastBlock;
}

void* sf_get_next_block(void * middleBlockHeader, size_t blockSz){
    sf_block *pastBlock = middleBlockHeader;
    pastBlock = (sf_block*)((char*)middleBlockHeader + blockSz);
    return pastBlock;
}


