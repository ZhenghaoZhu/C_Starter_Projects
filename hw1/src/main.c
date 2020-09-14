#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"
#include "dtmf.h"
#include "audio.h"
#include "goertzel.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    int validArgsRet = validargs(argc, argv);
    
    if(validArgsRet == -1 && global_options == 0x0){
        USAGE(*argv, EXIT_FAILURE);
        return EXIT_FAILURE;
    }

    if(global_options & 1){
        USAGE(*argv, EXIT_SUCCESS);
    }
    else if(global_options & 2){
        // printf("GENERATE OPTION \n");
        AUDIO_HEADER ah;
        ah.magic_number = AUDIO_MAGIC;
        ah.data_offset = AUDIO_DATA_OFFSET;
        // NOTE: Data size is "the actual number of bytes of audio sample data" when writing into a file
        // FIXME: Later, replace data_size by the actual number of bytes when creating the file
        ah.data_size = AUDIO_FRAME_RATE * AUDIO_BYTES_PER_SAMPLE;
        ah.encoding = PCM16_ENCODING;
        ah.sample_rate = AUDIO_FRAME_RATE;
        ah.channels = AUDIO_CHANNELS;
        audio_write_header(stdout, &ah);
    }
    else if(global_options & 4){
        // printf("DETECT OPTION \n");
        // double hz[] = {1633.0}; // ONLY FOR TESTING PURPOSES, WILL REMOVE IN FINAL BUILD
        // FILE *stdinCopy = stdin;
        // for(int i = 0; i < 1; i++){
        //     GOERTZEL_STATE gs;
        //     uint32_t samples_N = 1000;
        //     double goertzel_k = (hz[i]/8000) * samples_N;
        //     printf("Curr K: %lf \n", goertzel_k);
        //     // NOTE: Might need to change this base on flag values
        //     FILE *stdinP = stdinCopy;
        //     // int16_t offset = 0;
        //     int offsetInt = 0;
        //     for(int i = 0; i < 24; i++){
        //         fgetc(stdinP);
        //         // offset = 0;
        //     }
        //     goertzel_init(&gs, samples_N, goertzel_k);
        //     int16_t curX = 0;
        //     double doubleCurX = 0.0;
        //     for(int i = 0; i < 999; i++){ // goertzel_step
        //         audio_read_sample(stdinP, &curX);
        //         doubleCurX = (double)curX;
        //         doubleCurX /= INT16_MAX;
        //         // printf("Cur X #%i: %x \n", i, curX);
        //         goertzel_step(&gs, doubleCurX);
        //         curX = 0;
        //     }
        //     audio_read_sample(stdinP, &curX);
        //     doubleCurX = (double)curX;
        //     doubleCurX /= INT16_MAX;
        //     printf("LAST SAMPLE %lf \n", doubleCurX);
        //     double curStrength = goertzel_strength(&gs, doubleCurX);

        //     printf("Current Strength for %lf hz: %lf \n", hz[i], curStrength);
        // }
        // for(int i = 0; i < 10; i++){
        //     int16_t test1 = 0;
        //     audio_read_sample(stdin, &test1);
        //     printf("Test 1: %x \n", test1);
        // }
        

        int16_t curSample = 0;


        int8_t ch = 1;
        // if((ch = fgetc(stdin)) != EOF){
            // NOTE: Can't rewind as piping doesn't support it
            curSample = ch;
            audio_read_sample(stdin, &curSample);
        // } else {
            // printf("NO STDIN GIVEN. \n");
            // return EXIT_SUCCESS;
        // }
        
        // NOTE: It's fine if stdout is nothing as it will just print out to console.
        audio_write_sample(stdout, curSample);
        printf("\n");

        // printf("Current Sample: %p \n", pCurSample);
        // printf("%c", curSample);

        
        // AUDIO_HEADER ah;
        // ah.magic_number = 0;
        // ah.data_offset = 0;
        // ah.data_size = 0;
        // ah.encoding = 0;
        // ah.sample_rate = 0;
        // ah.channels = 0;
        // audio_read_header(stdin, &ah);
        // printf("1: %i \n", ah.magic_number);
        // printf("2: %i \n", ah.data_offset);
        // printf("3: %i \n", ah.data_size);
        // printf("4: %i \n", ah.encoding);
        // printf("5: %i \n", ah.sample_rate);
        // printf("6: %i \n", ah.channels);
    }

    // TO BE IMPLEMENTED
    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
