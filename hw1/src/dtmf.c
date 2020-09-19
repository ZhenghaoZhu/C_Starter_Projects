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
double getMax(double d1, double d2);
int check_SIX_DB(double mainStr, double str1, double str2, double str3);
int get_txt_idx(char *valPtr, int *currVal);
int getCurFreqColRow(char curDTMF, double *curStrRow, double *curStrCol);
void get_noise_w(double *noise_w);

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
 *  @param length  Number of audio samples to be written. (length = -t (val) * 8)
 *  @return 0 if the header and specified number of samples are written successfully,
 *  EOF otherwise.
 */
int dtmf_generate(FILE *events_in, FILE *audio_out, uint32_t length) {
    // TO BE IMPLEMENTED

    // Check for noise file avaialilibyt
    // Write header into audio_out
    FILE *noisePtr;
    if(noise_file != NULL){
        noisePtr = fopen(noise_file, "r");
        if(noisePtr == NULL){
            debug("Provided file doesn't exist \n");
            return EOF;
        }
    }
    // #define LINE_BUF_SIZE 80
    // char line_buf[LINE_BUF_SIZE];
    AUDIO_HEADER ah;
    ah.magic_number = AUDIO_MAGIC;
    ah.data_offset = AUDIO_DATA_OFFSET;
    ah.data_size =  length * 2; //  NOTE: data_size = -t val * 8 * 2 
    ah.encoding = PCM16_ENCODING;
    ah.sample_rate = AUDIO_FRAME_RATE;
    ah.channels = AUDIO_CHANNELS;
    audio_write_header(audio_out, &ah);

    int curDataSz = ah.data_size; // For padding ending, if necessary
    int prevStrIdx = -1;
    int prevEndIdx = -1;
    int curStrIdx = 0;
    int curEndIdx = 0;
    int16_t truncCurSample = 0;
    int16_t curNoiseSample = 0;
    double dcurNoiseSample = 0.0;
    double curStrRow = 0.0;
    double curStrCol = 0.0;
    double dcurSample = -1.0;
    double noise_w = 0.0;
    char *strLineBuf = line_buf;
    char curDTMF = 'z';
    int buffOffset = 0;
    bool noise_file_available = false;
    bool prevValChangeOnce = true;
    FILE *noise_file_p;
    noise_file_p = fopen(noise_file, "r");
    if(noise_file_p != NULL){
        noise_file_available = true;
    }
    get_noise_w(&noise_w);
    debug("NOISE W: %lf \n", noise_w);

    while(fgets(line_buf, LINE_BUF_SIZE, events_in) != NULL){
        strLineBuf = line_buf;
        buffOffset = get_txt_idx(strLineBuf, &curStrIdx) + 1;
        strLineBuf += buffOffset;
        buffOffset = get_txt_idx(strLineBuf, &curEndIdx) + 1;
        strLineBuf += buffOffset;
        curDTMF = *strLineBuf;
        getCurFreqColRow(curDTMF, &curStrRow, &curStrCol);

        if(curEndIdx < curStrIdx){
            return EOF;
        }
        if(prevStrIdx != -1 && prevEndIdx != -1){
            if(curStrIdx < prevEndIdx){
                return EOF;
            }
        } else if(prevValChangeOnce && curStrIdx > 0){
            prevStrIdx = 0;
            prevEndIdx = 0;
            prevValChangeOnce = false;
        }  

        // Padding for between gaps in txt file start and end index
        if(prevStrIdx != -1 && prevEndIdx != -1){
            for(int i = prevEndIdx; i < curStrIdx; i++){
                if(noise_file_available){
                    audio_read_sample(noise_file_p, &curNoiseSample);
                    dcurNoiseSample = ((double)curNoiseSample) * noise_w;
                    dcurSample = 0.0 * (1.0 - noise_w) + dcurNoiseSample * (noise_w);
                    dcurSample *= INT16_MAX;
                    truncCurSample = trunc(dcurSample);
                    audio_write_sample(audio_out, truncCurSample);
                }
                else {
                    truncCurSample = 0;
                    audio_write_sample(audio_out, truncCurSample);
                }
            }
        }
            

        // Write actual DTMF stuff
        for(int i = curStrIdx; i < curEndIdx; i++){
            if(noise_file_available){
                audio_read_sample(noise_file_p, &curNoiseSample);
                dcurNoiseSample = ((double)curNoiseSample) * noise_w;
                dcurSample = (cos((2.0 * M_PI * curStrRow * i) / AUDIO_FRAME_RATE ) * 0.5) + (cos((2.0 * M_PI * curStrCol * i) / AUDIO_FRAME_RATE ) * 0.5);
                dcurSample = dcurSample * (1.0 - noise_w) + dcurNoiseSample * (noise_w);
                dcurSample *= INT16_MAX;
                truncCurSample = trunc(dcurSample);
                audio_write_sample(audio_out, truncCurSample);
            }
            else {
                dcurSample = (cos((2.0 * M_PI * curStrRow * i) / AUDIO_FRAME_RATE ) * 0.5) + (cos((2.0 * M_PI * curStrCol * i) / AUDIO_FRAME_RATE ) * 0.5);
                dcurSample *= INT16_MAX;
                truncCurSample = trunc(dcurSample);
                audio_write_sample(audio_out, truncCurSample);
            }
        }

        prevStrIdx = curStrIdx;
        prevEndIdx = curEndIdx;
        
    }
    debug("CURDATSZ: %i prevEndIdx: %i \n", curDataSz, prevEndIdx);
    curDataSz /= 2;
    for(int i = prevEndIdx; i < curDataSz ; i++){
        truncCurSample = 0;
        audio_write_sample(audio_out, truncCurSample);
    }

    if(noise_file_available){
        fclose(noise_file_p);
    }

    return 0;
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
    AUDIO_HEADER ah;
    
    if(audio_read_header(audio_in, &ah) == EOF){
        return EOF;
    }

    int *freqPtr = dtmf_freqs;
    int R = 8000;
    int curBlockSize = 0;
    uint32_t N = block_size;
    int16_t startIdx = 0;
    int16_t endIdx = 0;
    int16_t prevStartIdx = -1;
    int16_t prevEndIdx = -1;
    int16_t curIdx = 0;
    double curK = 0.0;
    int prevRowTone = -1;
    int prevColTone = -1;
    int curRowTone = -1;
    int curColTone = -1;
    bool EOF_NOT_FOUND = true;
    
    while(EOF_NOT_FOUND){
        GOERTZEL_STATE gs_697, gs_770, gs_852, gs_941, gs_1209, gs_1336, gs_1477, gs_1633;
        
        curK = (((double)*freqPtr) * N)/R; // K = (F * N)/R
        freqPtr++;
        goertzel_init(&gs_697, N, curK);
        
        curK = (((double)*freqPtr) * N)/R;
        freqPtr++;
        goertzel_init(&gs_770, N, curK);
        
        curK = (((double)*freqPtr) * N)/R;
        freqPtr++;
        goertzel_init(&gs_852, N, curK);
        
        curK = (((double)*freqPtr) * N)/R;
        freqPtr++;
        goertzel_init(&gs_941, N, curK);
        
        curK = (((double)*freqPtr) * N)/R;
        freqPtr++;
        goertzel_init(&gs_1209, N, curK);
        
        curK = (((double)*freqPtr) * N)/R;
        freqPtr++;
        goertzel_init(&gs_1336, N, curK);
        
        curK = (((double)*freqPtr) * N)/R;
        freqPtr++;
        goertzel_init(&gs_1477, N, curK);
        
        curK = (((double)*freqPtr) * N)/R;
        freqPtr++;
        goertzel_init(&gs_1633, N, curK);

        freqPtr = dtmf_freqs;
        int16_t curX = 0;
        double doubleCurX = 0.0;

        for(int i = 0; i < (N - 1); i++){
            curBlockSize++;
            audio_read_sample(audio_in, &curX);
            if(curX == -1){
                EOF_NOT_FOUND = false;
                // curIdx -= N;
                break;
            }
            // Check for -1
            doubleCurX = (double)curX;
            doubleCurX /= INT16_MAX;
            // printf("Val #%i: %lf \n", i, doubleCurX);
            // Do all steps at the same time with curX
            goertzel_step(&gs_697, doubleCurX);
            goertzel_step(&gs_770, doubleCurX);
            goertzel_step(&gs_852, doubleCurX);
            goertzel_step(&gs_941, doubleCurX);

            goertzel_step(&gs_1209, doubleCurX);
            goertzel_step(&gs_1336, doubleCurX);
            goertzel_step(&gs_1477, doubleCurX);
            goertzel_step(&gs_1633, doubleCurX);
        }

        if(curBlockSize < (N - 1)){
            break;
        } 
        else {
            // Goertzel Strength
            curBlockSize = 0;
            curIdx += N;
            curBlockSize++;
            audio_read_sample(audio_in, &curX);
            doubleCurX = (double)curX;
            doubleCurX /= INT16_MAX;
            double str_697, str_770, str_852, str_941, str_1209, str_1336, str_1477, str_1633;
            
            str_697 = goertzel_strength(&gs_697, doubleCurX);
            str_770 = goertzel_strength(&gs_770, doubleCurX);
            str_852 = goertzel_strength(&gs_852, doubleCurX);
            str_941 = goertzel_strength(&gs_941, doubleCurX);

            str_1209 = goertzel_strength(&gs_1209, doubleCurX);
            str_1336 = goertzel_strength(&gs_1336, doubleCurX);
            str_1477 = goertzel_strength(&gs_1477, doubleCurX);
            str_1633 = goertzel_strength(&gs_1633, doubleCurX);

            double curStrongRow = getMax(getMax(str_697, str_770), getMax(str_852, str_941));
            double curStrongCol = getMax(getMax(str_1209, str_1336), getMax(str_1477, str_1633));
            bool rowRatioMinDB = false;
            bool colRatioMinDB = false;
            
            if(curStrongRow == str_697){
                if(check_SIX_DB(str_697, str_770, str_852, str_941)){
                    curRowTone = 0;
                    rowRatioMinDB = true;
                }
            } 
            else if(curStrongRow == str_770){
                if(check_SIX_DB(str_770, str_697, str_852, str_941)){
                    curRowTone = 4;
                    rowRatioMinDB = true;
                }
            } 
            else if(curStrongRow == str_852){
                if(check_SIX_DB(str_852, str_697, str_770, str_941)){
                    curRowTone = 8;
                    rowRatioMinDB = true;
                }
            } 
            else { // str_941
                if(check_SIX_DB(str_941, str_697, str_770, str_852)){
                    curRowTone = 12;
                    rowRatioMinDB = true;
                }
            }

            if(curStrongCol == str_1209){
                if(check_SIX_DB(str_1209, str_1336, str_1477, str_1633)){
                    curColTone = 0;
                    colRatioMinDB = true;
                }
            } 
            else if(curStrongCol == str_1336){
                if(check_SIX_DB(str_1336, str_1209, str_1477, str_1633)){
                    curColTone = 1;
                    colRatioMinDB = true;
                }
            } 
            else if(curStrongCol == str_1477){
                if(check_SIX_DB(str_1477, str_1209, str_1336, str_1633)){
                    curColTone = 2;
                    colRatioMinDB = true;
                }
            } 
            else { // str_1633
                if(check_SIX_DB(str_1633, str_1209, str_1336, str_1477)){
                    curColTone = 3;
                    colRatioMinDB = true;
                }
            }

            // Referencing Piazza Post #161, Checking if current block has DTMF tone
            if((curStrongRow + curStrongCol >= MINUS_20DB) && (curStrongRow/curStrongCol >= FOUR_DB * 0.01) && (curStrongRow/curStrongCol <= FOUR_DB) && rowRatioMinDB && colRatioMinDB){
                if(prevRowTone == -1 && prevColTone == -1){
                    prevRowTone = curRowTone;
                    prevColTone = curColTone;
                }
                else if(curRowTone == prevRowTone && curColTone == prevColTone){ // Same DTMF tone in this block, extend block
                    endIdx = curIdx;
                }
                else{
                    uint8_t *curToneChar = *dtmf_symbol_names;
                    curToneChar += (prevRowTone + prevColTone);
                    double blockLen = (double)(endIdx - startIdx);
                    if(blockLen/8000.0 >= MIN_DTMF_DURATION){
                        fprintf(events_out, "%i\t%i\t%c\n", startIdx, endIdx, *curToneChar);
                    }
                    prevRowTone = curRowTone;
                    prevColTone = curColTone;
                    startIdx = curIdx - N;
                    endIdx = curIdx;
                }
            }
        }
    }
    if(endIdx > 0){
        uint8_t *curToneChar = *dtmf_symbol_names;
        curToneChar += (curRowTone + curColTone);
        fprintf(events_out, "%i\t%i\t%c\n", startIdx, endIdx, *curToneChar);
    }
    
    return 0;
}

int check_SIX_DB(double mainStr, double str1, double str2, double str3){
    return((mainStr/str1 >= SIX_DB) && (mainStr/str2 >= SIX_DB) && (mainStr/str3 >= SIX_DB));
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
    int temp_noise_level = -1;
    int temp_block_size = -1;
    int temp_audio_samples = -1;

    tempArgv++; // Skip bin/dtmf to get to get to second argument

    /*Too few arguments*/
    if(argc < 2){
        return EOF;
    }
    if(get_current_flag(*tempArgv, &frstChar, &scndChar) > 3){
        // printf("INVALID COMMAND \n");
        return EOF;
    }
    tempArgv++;

    if(frstChar == '-' && scndChar == 'h'){
        // Help Mode
        global_options = HELP_OPTION;
        return 0;
    }


    /*Check first flag*/
    if(frstChar == '-' && scndChar == 'g'){
        bool tFlagSeen = false;
        bool lFlagSeen = false;
        bool nFlagSeen = false;
        // DONE: Generate mode
        for(i = 2; i < argc; i++){
            if(get_current_flag(*tempArgv, &frstChar, &scndChar) > 3){
                // printf("INVALID COMMAND \n");
                return EOF;
            }
            tempArgv++;
            if(frstChar != '-'){
                // Wrong flag character
                return EOF;
            } else if (scndChar == 't' && !tFlagSeen){
                // DONE: Check if nexr value is and int and valid based on range [0, UNIT32_MAX]
                tFlagSeen = true;
                i++;
                if(!(get_current_value(*tempArgv, &currVal))){
                    // printf("INVALID VALUE for -t \n");
                    return EOF;
                }
                tempArgv++;
                if(currVal < 0 || currVal > 268435455){
                    // printf("%li \n", currVal);
                    // printf("T Flag Value Out of Range \n");
                    return EOF;
                }
                temp_audio_samples = currVal; // 8 samples every MSEC
                // printf("T FLAG \n");
            } else if (scndChar == 'l' && !lFlagSeen){
                // DONE: Check if nexr value is and int and valid based on range [-30, 30]
                lFlagSeen = true;
                i++;
                if(!(get_current_value(*tempArgv, &currVal))){
                    // printf("INVALID VALUE for -l \n");
                    return EOF;
                }
                tempArgv++;
                if(currVal < -30 || currVal > 30){
                    // printf("L Flag Value Out of Range \n");
                    return EOF;
                }      
                temp_noise_level = currVal;
                // printf("L FLAG \n");        
            } else if (scndChar == 'n' && !nFlagSeen){
                nFlagSeen = true;
                i++;
                // printf("N FLAG \n");
                // DONE: Parse noise file name and put in variable
                noise_file = *tempArgv;
                tempPtr = *tempArgv;
                while(*tempPtr != '\0'){
                    tempPtr++;
                    noiseFileLen++;
                }
                tempArgv++;
                if(noiseFileLen < 4){
                    // printf("Filename too short \n");
                    return EOF;
                }
                
            } else {
                // Invalid flag
                // printf("INVALID FLAG(S) \n");
                return EOF;
            }
        }
        if(temp_audio_samples != -1){
            audio_samples = temp_audio_samples;
        } else {
            audio_samples = 1000;
        }

        if(temp_block_size != -1){
            block_size = temp_block_size;
        } else {
            block_size = 100;
        }

        if(temp_noise_level != -1){
            noise_level = temp_noise_level;
        } else {
            noise_level = 0;
        }

        if(noiseFileLen == 0){
            noise_file = NULL;
        }
        global_options = GENERATE_OPTION;
        return 0;
    } else if(frstChar == '-' && scndChar == 'd'){
        bool bFlagSeen = false;
        // DONE: Detect mode
        for(i = 2; i < argc; i++){
            if(get_current_flag(*tempArgv, &frstChar, &scndChar) > 3){
                // printf("INVALID COMMAND \n");
                return EOF;
            }
            tempArgv++;
            if(frstChar != '-'){
                // Wrong flag character
                return EOF;
            } else if (scndChar == 'b' && !bFlagSeen){
                // DONE: Check if nexr value is and int and valid based on range [10, 1000]
                bFlagSeen = true;
                i++;
                if(!(get_current_value(*tempArgv, &currVal))){
                    // printf("INVALID VALUE for -b \n");
                    return EOF;
                }
                tempArgv++;
                if(currVal < 10 || currVal > 1000){
                    // printf("%li \n", currVal);
                    // printf("B Flag Value Out of Range \n");
                    // TODO: unset_global_vars(); To unset all variables if errors encountered
                    return EOF;
                }
                temp_block_size = currVal;
                // printf("B FLAG");
            } else {
                // Invalid flag
                // printf("INVALID FLAG \n");
                return EOF;
            }
        }
        if(temp_audio_samples != -1){
            audio_samples = temp_audio_samples;
        } else {
            audio_samples = 1000;
        }

        if(temp_block_size != -1){
            block_size = temp_block_size;
        } else {
            block_size = 100;
        }

        if(temp_noise_level != -1){
            noise_level = temp_noise_level;
        } else {
            noise_level = 0;
        }

        if(noiseFileLen == 0){
            noise_file = NULL;
        }
        global_options = DETECT_OPTION;
        return 0;
    } else {
        /*Invalid first flag*/
        // printf("INVALID FLAG \n");
        return EOF;
    }

}

// DONE: Buggy with string_one and can't use brackets ([])
int check_string_equality(char *string_one, char *string_two)
{
    for(;;){
        if(*string_one == '\0' || *string_two == '\0'){
            if(*string_one != '\0'){
                return EOF;
            }
            else if(*string_two != '\0'){
                return EOF;
            }
            else {
                return 0;
            }
        }
        if(*string_one != *string_two){
            return EOF;
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
    int negValue = 1;
    if(valPtr == NULL){
        return 0;
    }

    while(*valPtr != '\0'){
        if((int)(*valPtr) == 45){
            negValue = -1;
            valPtr++;
        }
        else if((int)(*valPtr) > 57 || (int)(*valPtr) < 48){
            return 0;
        }
        else {
            *currVal += ((long)(*valPtr) - 48);
            *currVal *= 10;
            valPtr++;
        }
        
    }
    *currVal /= 10;
    *currVal *= negValue;
    return 1;
}

int string_copy(char *src, char *dest){
    if(dest == NULL){
        return EOF;
    }
    char *retPtr = dest;
    uint64_t srcLen = 0;
    while(*src != '\0'){
        *dest = 'a';
        *src = 'b';
        dest++;
        src++;
        srcLen++;
    }

    // *dest = '\0'; //End string
    dest -= srcLen;
    // printf("%p \n", dest);
    if(srcLen < 4){
        return EOF;
    }
    return srcLen;
}

double getMax(double d1, double d2){
    if(d1 > d2){
        return d1;
    }
    else {
        return d2;
    }
}

int get_txt_idx(char *valPtr, int *currVal){
    *currVal = 0;
    int count = 0;
    if(valPtr == NULL){
        return 0;
    }

    while(*valPtr != '\0' || *valPtr != '\t'){
        if((int)(*valPtr) > 57 || (int)(*valPtr) < 48){
            *currVal /= 10;
            return count;
        }
        *currVal += ((int)(*valPtr) - 48);
        *currVal *= 10;
        valPtr++;
        count++;
    }
    *currVal /= 10;
    return count;
}

int getCurFreqColRow(char curDTMF, double *curStrRow, double *curStrCol){
    switch(curDTMF) {

        case '1' :
            *curStrRow = 697.0;
            *curStrCol = 1209.0;
            break;
            
        case '2' :
            *curStrRow = 697.0;
            *curStrCol = 1336.0;
            break;

        case '3' :
            *curStrRow = 697.0;
            *curStrCol = 1477.0;
            break;
            
        case 'A' :
            *curStrRow = 697.0;
            *curStrCol = 1633.0;
            break;

        case '4' :
            *curStrRow = 770.0;
            *curStrCol = 1209.0;
            break;
            
        case '5' :
            *curStrRow = 770.0;
            *curStrCol = 1336.0;
            break;

        case '6' :
            *curStrRow = 770.0;
            *curStrCol = 1477.0;
            break;
            
        case 'B' :
            *curStrRow = 770.0;
            *curStrCol = 1633.0;
            break;
            
        case '7' :
            *curStrRow = 852.0;
            *curStrCol = 1209.0;
            break;

        case '8' :
            *curStrRow = 852.0;
            *curStrCol = 1336.0;
            break;
            
        case '9' :
            *curStrRow = 852.0;
            *curStrCol = 1477.0;
            break;

        case 'C' :
            *curStrRow = 852.0;
            *curStrCol = 1633.0;
            break;
            
        case '*' :
            *curStrRow = 941.0;
            *curStrCol = 1209.0;
            break;

        case '0' :
            *curStrRow = 941.0;
            *curStrCol = 1336.0;
            break;
            
        case '#' :
            *curStrRow = 941.0;
            *curStrCol = 1477.0;
            break;

        case 'D' :
            *curStrRow = 941.0;
            *curStrCol = 1633.0;
            break;
        
        default :
            *curStrRow = 0.0;
            *curStrCol = 0.0;
            return 0;
    }
    
    return 1;
}

void get_noise_w(double *noise_w){
    double level_d = (double)noise_level;
    *noise_w = (pow(10.0, (level_d/10.0)))/(1 + pow(10.0, (level_d/10.0)));
    return;
}