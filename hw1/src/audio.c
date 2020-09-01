#include <stdio.h>

#include "audio.h"
#include "debug.h"

int traverse_audio_file_header(FILE *curFile);
int check_audio_header(int magic_number, int data_offset, int data_size, int encoding, int sample_rate, int channels);

int audio_read_header(FILE *in, AUDIO_HEADER *hp) {
    FILE* curFile;
    
    // curFile = fopen("rsrc/941Hz_1sec.au", "r"); // Open file for only reading, return NULL if file doesn't exist
    curFile = in;
    if(curFile == NULL){
        printf("File inputted is NULL, please run program again and input correct absolute file path. \n");
        return -1;
    }

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
    // TO BE IMPLEMENTED
    return EOF;
}

// NOTE: int16_t because each frame is 2 bytes due to 16-bit and single channel configuration
int audio_read_sample(FILE *in, int16_t *samplep) {
    // TO BE IMPLEMENTED
    return EOF;
}

int audio_write_sample(FILE *out, int16_t sample) {
    // TO BE IMPLEMENTED
    return EOF;
}

int traverse_audio_file_header(FILE *curFile){
    int curHex = 0x0;
    int fullHex = 0x0;
    
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

int check_audio_header(int magic_number, int data_offset, int data_size, int encoding, int sample_rate, int channels){
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
