#include <stdio.h>
#include <stdlib.h>

#include "const.h"
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

int main(int argc, char **argv)
{
    int validArgsRet;
    validArgsRet = validargs(argc, argv);
    if(validArgsRet == -1){
        printf("INVALID ARGS \n");
        USAGE(*argv, EXIT_FAILURE);
        return EXIT_FAILURE;
    }
    if(global_options & 1){
        USAGE(*argv, EXIT_SUCCESS);
        return EXIT_SUCCESS;
    }

    if(validArgsRet == 0){
        printf("Not help flag \n");
        return EXIT_SUCCESS;
    }
    // TO BE IMPLEMENTED
    global_options = 0x0;
    return EXIT_FAILURE;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
