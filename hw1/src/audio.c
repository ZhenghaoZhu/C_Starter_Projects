#include <stdio.h>

#include "audio.h"
#include "debug.h"

int traverse_audio_file_header(FILE *curFile);
int check_audio_header(uint32_t magic_number, uint32_t data_offset, uint32_t data_size, uint32_t encoding, uint32_t sample_rate, uint32_t channels);
int helper_audio_write_header(FILE *out, uint32_t curVar);

int audio_read_header(FILE *in, AUDIO_HEADER *hp) {
    FILE* curFile;
    
    // curFile = fopen("rsrc/941Hz_1sec.au", "r"); // Open file for only reading, return NULL if file doesn't exist
    if(in == NULL){
        printf("File inputted is NULL, please run program again and input correct absolute file path. \n");
        return EOF;
    }
    curFile = in;
    

    // NOTE: data_offset for writing into will always be 24. When reading, it can be varied.    
    hp->magic_number = traverse_audio_file_header(curFile);
    // printf("Magic Number: %i \n", hp->magic_number);
    hp->data_offset = traverse_audio_file_header(curFile);
    // printf("Data Offset: %i \n", hp->data_offset);
    hp->data_size = traverse_audio_file_header(curFile);
    // printf("Data Size: %i \n", hp->data_size);
    hp->encoding = traverse_audio_file_header(curFile);
    // printf("Encoding: %i \n", hp->encoding);
    hp->sample_rate = traverse_audio_file_header(curFile);
    // printf("Sample Rate: %i \n", hp->sample_rate);
    hp->channels = traverse_audio_file_header(curFile);
    // printf("Channels: %i \n", hp->channels);

    if(!check_audio_header(hp->magic_number, hp->data_offset, hp->data_size, hp->encoding, hp->sample_rate, hp->channels)){
        printf("AUDIO HEADER CHECK NOT PASSED! \n");
        return EOF;
    }

    
    // closes the file pointed by demo 
    fclose(curFile);
    printf("AUDIO HEADER CHECK PASSED! \n");
    return 0; 
    // return EOF;
}

int audio_write_header(FILE *out, AUDIO_HEADER *hp) {

    // NOTE: data_offset, for writing into, will always be 24. When reading, it can be varied.
    if(out == NULL || feof(out)){
        return EOF;
    }

    helper_audio_write_header(out, hp->magic_number);
    helper_audio_write_header(out, hp->data_offset);
    helper_audio_write_header(out, hp->data_size);
    helper_audio_write_header(out, hp->encoding);
    helper_audio_write_header(out, hp->sample_rate);
    helper_audio_write_header(out, hp->channels);

    return 0;
}

// NOTE: int16_t because each frame is 2 bytes due to 16-bit and single channel configuration
int audio_read_sample(FILE *in, int16_t *samplep) {
    // TODO: Get offset from header to get to the right position of sample
    
    FILE *curFile;

    curFile = in;
    int16_t curHex = 0x0;
    int16_t fullHex = 0x0;
    
    if((curHex = fgetc(curFile)) != EOF){
        fullHex ^= curHex;
        fullHex = fullHex << 8;
    } else {
        return EOF;
    }

    curHex = 0x0;
    
    if((curHex = fgetc(curFile)) != EOF){
        fullHex ^= curHex;
    } else {
        printf("EOF \n");
        return EOF;
    }

    // printf("FULL HEX: %x \n", fullHex);
    *samplep = fullHex;

    return 0;
}

int audio_write_sample(FILE *out, int16_t sample) {
    
    FILE *curFile;
    curFile = out;

    fprintf(curFile, "%c", ((sample & 0xFF00) >> 8));
    fprintf(curFile, "%c", (sample & 0x00FF));

    return 0;
}

int traverse_audio_file_header(FILE *curFile){
    uint8_t curHex = 0x0;
    uint32_t fullHex = 0x0;
    
    curHex = fgetc(curFile);
    fullHex ^= curHex;
    fullHex = fullHex << 8;

    curHex = fgetc(curFile);
    fullHex ^= curHex;
    fullHex = fullHex << 8;

    curHex = fgetc(curFile);
    fullHex ^= curHex;
    fullHex = fullHex << 8;

    curHex = fgetc(curFile);
    fullHex ^= curHex;

    return fullHex;
}

int check_audio_header(uint32_t magic_number, uint32_t data_offset, uint32_t data_size, uint32_t encoding, uint32_t sample_rate, uint32_t channels){
    if(magic_number != AUDIO_MAGIC){
        return 0;
    }
    if(data_offset < AUDIO_DATA_OFFSET){
        return 0;
    }

    // NOTE: No need to check for data_size when reading a file, just go until EOF is seen.

    if(encoding != PCM16_ENCODING){
        return 0;
    }
    if(sample_rate != AUDIO_FRAME_RATE){
        return 0;
    }
    if(channels != AUDIO_CHANNELS){
        return 0;
    }

    return 1;

}

int helper_audio_write_header(FILE *out, uint32_t curVar){
    FILE *curFile = out;
    uint32_t curHex = 0x0;
    int count = 3;
    uint32_t curShift = 0;

    while(count > -1){
        curHex = 0x0;
        curHex ^= curVar;
        curShift = 8 * count;
        curHex = curHex >> curShift;
        fprintf(curFile, "%c", curHex);
        count--;
    }

    return 0;
}
