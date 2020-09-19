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
        dtmf_generate(stdin, stdout, audio_samples * 8);
    }
    else if(global_options & 4){
        dtmf_detect(stdin, stdout);
    } 
    else {
        return EXIT_FAILURE;
    }

    // TO BE IMPLEMENTED
    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
