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


sf_block *globalEnd;

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

            if(curListBlockSz == blockSize){
                sf_quick_lists[i].length -= 1;
                sf_block *retBlock = sf_quick_lists[i].first;
                sf_quick_lists[i].first = retBlock->body.links.next;
                return retBlock->body.payload;
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
                    if((unsigned long)splitBlock > (unsigned long)globalEnd){
                        globalEnd = splitBlock;
                    }
                    return curBlock->body.payload;
                }
                else {
                    sf_remove_from_free_list(curBlock);
                    if(((globalEnd->header ^ MAGIC) & THIS_BLOCK_ALLOCATED) == THIS_BLOCK_ALLOCATED){
                        curBlock->header = ((curBlock->header ^ MAGIC) | THIS_BLOCK_ALLOCATED | PREV_BLOCK_ALLOCATED) ^ MAGIC;
                    }
                    else {
                        curBlock->header = ((curBlock->header ^ MAGIC) | THIS_BLOCK_ALLOCATED) ^ MAGIC;
                    }
                    if((unsigned long)curBlock > (unsigned long)globalEnd){
                        globalEnd = curBlock;
                    }
                    return curBlock->body.payload;
                }
            }
        }
    }

    sf_header globalEndHeader = globalEnd->header ^ MAGIC;
    sf_header globalEndBlockSz = globalEndHeader & ~0x7;
    bool prevAllForNewPage = false;
    if((globalEndHeader & THIS_BLOCK_ALLOCATED) == THIS_BLOCK_ALLOCATED){
        prevAllForNewPage = true;
    }
    sf_block *newPage = sf_mem_end() - 16;
    if(prevAllForNewPage){
        newPage->header = (PAGE_SZ | PREV_BLOCK_ALLOCATED) ^ MAGIC; 
        newPage->prev_footer = globalEndBlockSz ^ MAGIC;
    }
    else {
        newPage->header = (PAGE_SZ) ^ MAGIC;
        newPage->prev_footer = globalEndBlockSz ^ MAGIC;
    }

    size_t tempSize = 0;
    if(((globalEnd->header ^ MAGIC) & THIS_BLOCK_ALLOCATED) == THIS_BLOCK_ALLOCATED){
        tempSize = size;
    }
    else {
        tempSize = size - globalEndBlockSz;
    }
    while(((int)tempSize) >= 0){
        if(sf_mem_grow() == NULL){
            sf_errno = ENOMEM;
            return NULL;
        }
        else {
            sf_header curHeader = PAGE_SZ ^ MAGIC;
            newPage->header = curHeader;
            sf_set_block_footer(newPage);
            sf_coalesce(newPage, sf_get_block_sz(newPage), newPage->prev_footer ^ MAGIC, true);
            tempSize -= PAGE_SZ;
        }
    }

    return sf_malloc(size);
}

// NOTE: Page 885 for example code
void sf_free(void *pp) {
    
    sf_block *curBlock = (sf_block*)(pp - sizeof(char) * 16);
    // sf_show_block(curBlock);
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

    if(((curBlock->header ^ MAGIC) & PREV_BLOCK_ALLOCATED) == 0){
        sf_block *past_block = sf_get_past_block(curBlock, curBlockSz);
        unsigned long newCurBlockInt = (unsigned long) past_block;
        if((newCurBlockInt >= (unsigned long)sf_mem_start()) && (newCurBlockInt < (unsigned long)sf_mem_end()) && (((past_block->header ^ MAGIC) & THIS_BLOCK_ALLOCATED) != 0)){
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
    // sf_put_in_free_list(curBlock, curBlockSz);
    sf_coalesce(curBlock, curBlockSz, (curBlock->prev_footer ^ MAGIC) & ~0x7, false);


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
    
    if(((curBlock->header ^ MAGIC) & PREV_BLOCK_ALLOCATED) == 0){
        sf_block *past_block = sf_get_past_block(curBlock, curBlockSz);
        curBlockInt = (long int)past_block;
        if((curBlockInt >= (unsigned long)sf_mem_start()) && (((past_block->header ^ MAGIC) & THIS_BLOCK_ALLOCATED) != 0)){
            sf_errno = EINVAL;
            abort();
        }
    }

    if(rsize == 0){
        sf_free(pp);
        return NULL;
    }

    size_t difference = rsize + 8;
    while(difference % 16 != 0){
        difference += 1;
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
        sf_free((sf_block*)((char*)curBlock + 16));
        return newBiggerBlock; 
    }
    
    if(curBlockSz > rsize){ // TODO: Give smaller block than original
        if(curBlockSz - difference < 32){ // Splinter don't split
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
            if((unsigned long)splitBlock > (unsigned long)globalEnd){
                globalEnd = splitBlock;
            }
            sf_coalesce(splitBlock, (curBlockSz - newBlockSz), (splitBlock->prev_footer ^ MAGIC) & ~0x7, false);
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

void sf_flush_quick_list(sf_block* curBlock, int listIdx){
    sf_block *blockArray[QUICK_LIST_MAX];
    int count = 0;
    if (sf_quick_lists[listIdx].length != 0){
        sf_block *head = sf_quick_lists[listIdx].first;
        while(head != NULL){
            head->header = ((head->header ^ MAGIC) & ~0x7) ^ MAGIC;
            sf_set_block_footer(head);
            blockArray[count] = head;
            count += 1;
            head = head->body.links.next;
        }
        curBlock->body.links.next = NULL;
        sf_quick_lists[listIdx].length = 1;
        sf_quick_lists[listIdx].first = curBlock;
        for(int i = 0; i < count; i++){
            sf_coalesce(blockArray[i], sf_get_block_sz(blockArray[i]), blockArray[i]->prev_footer, false);
        }
        return;
    }
    return;
}

void sf_flush_free_helper(sf_block* curBlock){
    curBlock->header = sf_get_block_sz(curBlock) ^ MAGIC;
    sf_set_block_footer(curBlock);
    sf_put_in_free_list(curBlock, sf_get_block_sz(curBlock));
    return;
}

void sf_init_first_page(){
    sf_init_lists();
    sf_block* pgBlock = (sf_block*) sf_mem_grow();
    sf_header pgBlockSz = 4080;
    pgBlockSz ^= MAGIC;
    pgBlock->header = pgBlockSz;
    pgBlock->prev_footer = 0;
    void* setFooter = pgBlock;
    setFooter += sizeof(char) * 4080;
    *((long int *) setFooter) = (long int) pgBlockSz;
    globalEnd = sf_mem_start();
    sf_free_list_heads[7].body.links.next = pgBlock;
    pgBlock->body.links.prev = &sf_free_list_heads[7];
    pgBlock->body.links.next = &sf_free_list_heads[7];

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

void sf_coalesce(void* middleBlock, size_t blockSz, size_t prevFooter, bool makingLargePage){
    
    size_t newSize = 0;
    size_t past_block_sz = 0;
    size_t next_block_sz = 0;
    sf_block * past_block = NULL;
    if(prevFooter > 0){
        past_block = sf_get_past_block(middleBlock, prevFooter);
    }
    sf_block * next_block = sf_get_next_block(middleBlock, blockSz);
    bool past_block_all = true;
    bool next_block_all = true;
    
    if(((unsigned long)past_block) < (unsigned long)sf_mem_start() || ((unsigned long)past_block) >= (unsigned long)((char*)sf_mem_end() - 16)){
        past_block = NULL;
        past_block_all = false;
    }
    else {
        past_block_sz = (past_block->header ^ MAGIC) & ~0x7;
        if(!((past_block->header ^ MAGIC) & THIS_BLOCK_ALLOCATED)){
            past_block_all = true;
        } else {
            past_block_all = false;
        }
    }

    if(((unsigned long)next_block) >= (unsigned long)((char*)sf_mem_end() - 16) || ((unsigned long)next_block) <= (unsigned long)sf_mem_start()){
        next_block = NULL;
        next_block_all = false;
    }
    else {
        next_block_sz = (next_block->header ^ MAGIC) & ~0x7;
        if(!((next_block->header ^ MAGIC) & THIS_BLOCK_ALLOCATED)){
            next_block_all = true;
        } else {
            next_block_all = false;
        }
    }

    if(makingLargePage){
        next_block = NULL;
        next_block_all = false;
    }

    if(!past_block_all && !next_block_all){
        sf_put_in_free_list(middleBlock, blockSz);
        return;
    }
    else if(past_block_all && !next_block_all){
        sf_remove_from_free_list(middleBlock);
        sf_remove_from_free_list(past_block);
        newSize = blockSz + past_block_sz;
        sf_block *newBlock = (sf_block *)past_block;
        newBlock->header = (newSize | PREV_BLOCK_ALLOCATED) ^ MAGIC;
        newBlock->prev_footer = (past_block_sz | THIS_BLOCK_ALLOCATED) ^ MAGIC;
        sf_set_block_footer(newBlock);
        sf_put_in_free_list(newBlock, newSize);
    }
    else if(!past_block_all && next_block_all){
        sf_remove_from_free_list(middleBlock);
        sf_remove_from_free_list(next_block);
        newSize = blockSz + next_block_sz;
        sf_block *newBlock = (sf_block *)middleBlock;
        newBlock->header = (newSize | PREV_BLOCK_ALLOCATED) ^ MAGIC;
        newBlock->prev_footer = (past_block_sz | THIS_BLOCK_ALLOCATED) ^ MAGIC;
        sf_set_block_footer(newBlock);
        sf_put_in_free_list(newBlock, newSize);
        
    }
    else {
        sf_remove_from_free_list(middleBlock);
        sf_remove_from_free_list(past_block);
        sf_remove_from_free_list(next_block);
        newSize = blockSz + past_block_sz + next_block_sz;
        sf_block *newBlock = (sf_block *)past_block;
        newBlock->header = (newSize | PREV_BLOCK_ALLOCATED) ^ MAGIC;
        newBlock->prev_footer = (past_block_sz | THIS_BLOCK_ALLOCATED) ^ MAGIC;
        sf_set_block_footer(newBlock);
        sf_put_in_free_list(newBlock, newSize);
    }
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
        sf_flush_quick_list(curBlock, curIdx); // Should reset length here
        return;
    }
    if(sf_quick_lists[curIdx].first != NULL){
        sf_quick_lists[curIdx].length += 1; // Add to quicklist
        if(sf_quick_lists[curIdx].first == NULL){
            curBlock->body.links.next = NULL;
        }
        else {
            sf_block *oldFirst = sf_quick_lists[curIdx].first;
            curBlock->body.links.next = oldFirst;   
        curBlock->body.links.next = oldFirst;
            curBlock->body.links.next = oldFirst;   
        }

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

void sf_remove_from_free_list(sf_block *removedBlock){
    int i;
    sf_block *curBlock;
    for(i = 0; i < NUM_FREE_LISTS; i++){
        curBlock = &sf_free_list_heads[i]; // Sentinel
        while(curBlock->body.links.next != &sf_free_list_heads[i]){
            curBlock = curBlock->body.links.next;
            if((unsigned long)curBlock == (unsigned long)removedBlock){
                sf_block *tempBlock = curBlock->body.links.next;
                curBlock->body.links.next->body.links.prev = curBlock->body.links.prev;
                curBlock->body.links.prev->body.links.next = tempBlock;
                return;
            }
        }
    }
}

size_t sf_get_block_sz(sf_block *curBlock){
    return ((curBlock->header ^ MAGIC) & ~0x7);
}