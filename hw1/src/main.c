#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"
#include "dtmf.h"
#include "audio.h"

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
    
    if(validArgsRet == -1){
        printf("FAILURE \n");
        USAGE(*argv, EXIT_FAILURE);
    }
    
    if(global_options & 1){
        printf("SUCCESS \n");
        USAGE(*argv, EXIT_SUCCESS);
    }
    else if(global_options & 2){
        printf("GENERATE OPTION \n");
    }
    else if(global_options & 4){
        printf("DETECT OPTION \n");
        AUDIO_HEADER ah;
        ah.magic_number = 0;
        ah.data_offset = 0;
        ah.data_size = 0;
        ah.encoding = 0;
        ah.sample_rate = 0;
        ah.channels = 0;
        audio_read_header(stdin, &ah);
    }

    // TO BE IMPLEMENTED
    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
