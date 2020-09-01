#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "const.h"
#include "audio.h"
#include "dtmf.h"
#include "dtmf_static.h"
#include "goertzel.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int get_current_flag(char *flagPtr, char *frstChar, char *scndChar);
int get_current_value(char *valPtr, long *currVal);
int string_copy(char *src, char *dest);
int check_string_equality(char *string_one, char *string_two);
int get_file_header(FILE *file);

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the pathname of the current file or directory
 * as well as other data have been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 */

/**
 * DTMF generation main function.
 * DTMF events are read (in textual tab-separated format) from the specified
 * input stream and audio data of a specified duration is written to the specified
 * output stream.  The DTMF events must be non-overlapping, in increasing order of
 * start index, and must lie completely within the specified duration.
 * The sample produced at a particular index will either be zero, if the index
 * does not lie between the start and end index of one of the DTMF events, or else
 * it will be a synthesized sample of the DTMF tone corresponding to the event in
 * which the index lies.
 *
 *  @param events_in  Stream from which to read DTMF events.
 *  @param audio_out  Stream to which to write audio header and sample data.
 *  @param length  Number of audio samples to be written.
 *  @return 0 if the header and specified number of samples are written successfully,
 *  EOF otherwise.
 */
int dtmf_generate(FILE *events_in, FILE *audio_out, uint32_t length) {
    // TO BE IMPLEMENTED
    return EOF;
}

/**
 * DTMF detection main function.
 * This function first reads and validates an audio header from the specified input stream.
 * The value in the data size field of the header is ignored, as is any annotation data that
 * might occur after the header.
 *
 * This function then reads audio sample data from the input stream, partititions the audio samples
 * into successive blocks of block_size samples, and for each block determines whether or not
 * a DTMF tone is present in that block.  When a DTMF tone is detected in a block, the starting index
 * of that block is recorded as the beginning of a "DTMF event".  As long as the same DTMF tone is
 * present in subsequent blocks, the duration of the current DTMF event is extended.  As soon as a
 * block is encountered in which the same DTMF tone is not present, either because no DTMF tone is
 * present in that block or a different tone is present, then the starting index of that block
 * is recorded as the ending index of the current DTMF event.  If the duration of the now-completed
 * DTMF event is greater than or equal to MIN_DTMF_DURATION, then a line of text representing
 * this DTMF event in tab-separated format is emitted to the output stream. If the duration of the
 * DTMF event is less that MIN_DTMF_DURATION, then the event is discarded and nothing is emitted
 * to the output stream.  When the end of audio input is reached, then the total number of samples
 * read is used as the ending index of any current DTMF event and this final event is emitted
 * if its length is at least MIN_DTMF_DURATION.
 *
 *   @param audio_in  Input stream from which to read audio header and sample data.
 *   @param events_out  Output stream to which DTMF events are to be written.
 *   @return 0  If reading of audio and writing of DTMF events is sucessful, EOF otherwise.
 */
int dtmf_detect(FILE *audio_in, FILE *events_out) {
    // TO BE IMPLEMENTED
    return EOF;
}

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the operation mode of the program (help, generate,
 * or detect) will be recorded in the global variable `global_options`,
 * where it will be accessible elsewhere in the program.
 * Global variables `audio_samples`, `noise file`, `noise_level`, and `block_size`
 * will also be set, either to values derived from specified `-t`, `-n`, `-l` and `-b`
 * options, or else to their default values.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected program operation mode, and global variables `audio_samples`,
 * `noise file`, `noise_level`, and `block_size` to contain values derived from
 * other option settings.
 */
int validargs(int argc, char **argv)
{
    // TO BE IMPLEMENTED
    uint32_t i;
    long currVal = 0;
    int noiseFileLen = 0;
    char frstChar = '0';
    char scndChar = '0';
    char *tempPtr = NULL;
    char **tempArgv = argv;  // pointer to pointer, not pointer to char
    global_options = 0x0;
    noise_level = 0;
    block_size = 100;
    audio_samples = 1000;
    noise_file = NULL;
    // DONE: Put default value for noise_file

    tempArgv++; // Skip bin/dtmf to get to get to second argument

    /*Too few arguments*/
    if(argc < 2){
        return -1;
    }
    if(get_current_flag(*tempArgv, &frstChar, &scndChar) > 3){
        printf("INVALID COMMAND \n");
        return -1;
    }
    tempArgv++;

    if(frstChar == '-' && scndChar == 'h'){
        // Help Mode
        global_options = HELP_OPTION;
        return 0;
    }

    // /*Too many arguments*/
    // if(argc > 8){
    //     return -1;
    // }

    /*Check first flag*/
    if(frstChar == '-' && scndChar == 'g'){
        // DONE: Generate mode
        for(i = 2; i < argc; i++){
            if(get_current_flag(*tempArgv, &frstChar, &scndChar) > 3){
                printf("INVALID COMMAND \n");
                return -1;
            }
            tempArgv++;
            if(frstChar != '-'){
                // Wrong flag character
                return -1;
            } else if (scndChar == 't'){
                // DONE: Check if nexr value is and int and valid based on range [0, UNIT32_MAX]
                i++;
                if(!(get_current_value(*tempArgv, &currVal))){
                    printf("INVALID VALUE for -t \n");
                    return -1;
                }
                tempArgv++;
                if(currVal < 0 || currVal > MAX_MSEC){
                    printf("%li \n", currVal);
                    printf("T Flag Value Out of Range \n");
                    return -1;
                }
                audio_samples = currVal*8; // 8 samples every MSEC
                printf("T FLAG \n");
            } else if (scndChar == 'l'){
                // DONE: Check if nexr value is and int and valid based on range [-30, 30]
                i++;
                if(!(get_current_value(*tempArgv, &currVal))){
                    printf("INVALID VALUE for -l \n");
                    return -1;
                }
                tempArgv++;
                if(currVal < -30 || currVal > 30){
                    printf("L Flag Value Out of Range \n");
                    return -1;
                }      
                noise_level = currVal;
                printf("L FLAG \n");        
            } else if (scndChar == 'n'){
                printf("N FLAG \n");
                // DONE: Parse noise file name and put in variable
                noise_file = *tempArgv;
                tempPtr = *tempArgv;
                while(*tempPtr != '\0'){
                    tempPtr++;
                    noiseFileLen++;
                }
                if(noiseFileLen < 4){
                    printf("Filename too short \n");
                    return -1;
                }
                i++;
            } else {
                // Invalid flag
                printf("INVALID FLAG \n");
                return -1;
            }
        }
        global_options = GENERATE_OPTION;
        return 0;
    } else if(frstChar == '-' && scndChar == 'd'){
        // DONE: Detect mode
        for(i = 2; i < argc; i++){
            if(get_current_flag(*tempArgv, &frstChar, &scndChar) > 3){
                printf("INVALID COMMAND \n");
                return -1;
            }
            tempArgv++;
            if(frstChar != '-'){
                // Wrong flag character
                return -1;
            } else if (scndChar == 'b'){
                // DONE: Check if nexr value is and int and valid based on range [10, 1000]
                i++;
                if(!(get_current_value(*tempArgv, &currVal))){
                    printf("INVALID VALUE for -b \n");
                    return -1;
                }
                tempArgv++;
                if(currVal < 10 || currVal > 1000){
                    printf("%li \n", currVal);
                    printf("B Flag Value Out of Range \n");
                    // TODO: unset_global_vars(); To unset all variables if errors encountered
                    return -1;
                }
                block_size = currVal;
                printf("B FLAG \n");
            } else {
                // Invalid flag
                printf("INVALID FLAG \n");
                return -1;
            }
        }
        global_options = DETECT_OPTION;
        return 0;
    } else {
        /*Invalid first flag*/
        printf("INVALID FLAG \n");
        return -1;
    }
}

// DONE: Buggy with string_one and can't use brackets ([])
int check_string_equality(char *string_one, char *string_two)
{
    for(;;){
        if(*string_one == '\0' || *string_two == '\0'){
            if(*string_one != '\0'){
                printf("NOT EQUAL \n");
                return -1;
            }
            else if(*string_two != '\0'){
                printf("NOT EQUAL \n");
                return -1;
            }
            else {
                printf("Strings are equal \n");
                return 0;
            }
        }
        if(*string_one != *string_two){
            printf("NOT EQUAL \n");
            return -1;
        }
        string_one++;
        string_two++;
    }

}


int get_current_flag(char *flagPtr, char *frstChar, char *scndChar){
    uint32_t flagLen = 0;
    while (*flagPtr != '\0'){
        *frstChar = *flagPtr;
        if(*(++flagPtr) == '\0'){
            return ++flagLen;
        }
        *scndChar = *flagPtr;
        if(*(++flagPtr) == '\0'){
            return ++flagLen;
        }
        flagLen += 3;
    }
    return flagLen;
}

int get_current_value(char *valPtr, long *currVal){
    *currVal = 0;
    while(*valPtr != '\0'){
        if((int)(*valPtr) > 57 || (int)(*valPtr) < 48){
            return 0;
        }
        *currVal += ((long)(*valPtr) - 48);
        *currVal *= 10;
        valPtr++;
    }
    *currVal /= 10;
    return 1;
}

int string_copy(char *src, char *dest){
    if(dest == NULL){
        return -1;
    }
    char *retPtr = dest;
    uint64_t srcLen = 0;
    while(*src != '\0'){
        printf("Test 1 \n");
        *dest = 'a';
        printf("Test 2 \n");
        *src = 'b';
        dest++;
        src++;
        srcLen++;
    }

    // *dest = '\0'; //End string
    dest -= srcLen;
    printf("%p \n", dest);
    if(srcLen < 4){
        return -1;
    }
    return srcLen;
}

int get_file_header(FILE *file){
    FILE* curFile;
    uint32_t display;
    uint32_t count = 0;
    // curFile = fopen("rsrc/941Hz_1sec.au", "r"); // Open file for only reading, return NULL if file doesn't exist
    curFile = file;
    if(curFile == NULL){
        printf("File inputted is NULL, please run program again and input correct absolute file path. \n");
        return -1;
    }
    while (1) { 
        // reading file 
        display = fgetc(curFile); 
  
        // end of file indicator 
        if (feof(curFile)) 
            break; 
  
        // displaying every characters
        if(display == 0){
            printf("00 ");
        }
        else if(display < 16){
            printf("0%x ", display);
        }
        else {
            printf("%x ", display);
        }

        count++;
        if(count >= 16){
            printf("\n");
            count = 0;
        } 
    } 
  
    // closes the file pointed by demo 
    fclose(curFile); 
    return 0;
    // TODO: Look more into when to call detect mode 
    // TODO: Look into when to call functions in audio.c 
}
