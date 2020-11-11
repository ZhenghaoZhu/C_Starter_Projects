/*
 * Legion: Command-line interface
 */

#include "legion.h"
#include "cli.h"
#include <debug.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>

bool running = true;
char* args[ARGS_ARRAY_LENGTH];
int argsArrayLen = 0;

void run_cli(FILE *in, FILE *out)
{
    // pid_t cpid;
    // if(fork() == 0){
    //     exit(0);
    // }
    // else {
    //     cpid = wait(NULL);
    // }
    
    // TODO  Establish all signal handlers before doing anything

    struct daemonNode *daemonNodeHead = (struct daemonNode*) malloc(sizeof(struct daemonNode));
    strncpy(daemonNodeHead->daemonName, "test", DAEMON_NAME_MAX_LENGTH);
    args[0] = "testsetsttstt";

    while(running){
        char cliArgs[CLI_ARGs_MAX_LENGTH];
        printf("legion> ");
        fflush(out);
        // char test = '\0';
        // while(!feof(in)){
        //     test = fgetc(in);
        //     if((int)test == 10){
        //         break;
        //     }
        //     printf("%i ", test);
        // }
        fgets(cliArgs, CLI_ARGs_MAX_LENGTH, in);
        legion_parse_args(cliArgs, out, daemonNodeHead);
        legion_quit(daemonNodeHead);
    }
    return;
}

/* SPECIFICATION  Single quote characters (') are treated specially: when such a character is encountered, all the subsequent characters up to the next single quote (or the end of the line, if there
is no other single quote character) are treated as a single field, regardless of whether any of those characters are spaces.  The quote characters themselves do not become part of the field. */

void legion_parse_args(char curStr[], FILE *out, struct daemonNode *daemonNodeHead){
    // char curArg = '\0';
    char *wholeStr = curStr;
    int curIdx = 0;
    argsArrayLen = 0;

    for(;;){
        if((int)wholeStr[curIdx] == 10){
            break;
        }
        printf("%i ", wholeStr[curIdx++]);
    }
    printf("\n");

    return;
}

int legion_char_idx(char curStr[]){
    return 0;
}

void legion_init(){
    return;
}

void legion_quit(struct daemonNode *daemonNodeHead){
    // TODO  Remove all daemons from processes
    // legion_free_daemon_node_head(daemonNodeHead); // Free daemon node list
    // sf_fini();
    running = false;
}

void legion_free_daemon_node_head(struct daemonNode *daemonNodeHead){
    struct daemonNode *tempNode = NULL;
    while(daemonNodeHead != NULL){
        tempNode = daemonNodeHead;
        daemonNodeHead = daemonNodeHead->nextDaemon;
        free(tempNode);
    }
}

void legion_help(){
    return;
}

void legion_register(char* curDaemon){
    return;
}

void legion_unregister(char* daemonName, char* daemonExec){
    return;
}

void legion_status(char* curDaemon){
    return;
}

void legion_status_all(){
    return;
}

void legion_start(char* daemonName){
    return;
}

void legion_stop(char* daemonName){
    return;
}

void legion_logrotate(char* curDaemon){
    return;
}

void print_word_chars(char* curWord){
    while(true){
        if((*curWord) == '\0'){
            printf("\n");
            return;
        }
        printf("%i ", *curWord);
        curWord++;
    }
}