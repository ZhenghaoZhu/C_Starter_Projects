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
bool sigchld_flag = false;
bool sigalrm_flag = false;
char *argArr[CUR_ARG_MAX_LENGTH];
int argArrIdx = 0;
int argsArrayLen = 0;
struct daemonNode *daemonNodeHead;

void run_cli(FILE *in, FILE *out)
{
    // TODO  Establish all signal handlers before doing anything
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);
    signal(SIGALRM, sigalrm_handler);

    daemonNodeHead = (struct daemonNode*) malloc(sizeof(struct daemonNode));
    strncpy(daemonNodeHead->daemonName, "daemonNodeHead", DAEMON_NAME_MAX_LENGTH);
    strncpy(daemonNodeHead->daemonExe, "daemonNodeHead", DAEMON_EXE_MAX_LENGTH);
    daemonNodeHead->numberOfArgs = 0;
    daemonNodeHead->daemonStatus = -1;
    daemonNodeHead->daemonProcessID = -1;
    daemonNodeHead->nextDaemon = NULL;

    while(running){
        char cliArgs[CLI_ARGs_MAX_LENGTH];
        fprintf(out, "%s", "legion> ");
        fflush(out);
        if(fgets(cliArgs, CLI_ARGs_MAX_LENGTH, in) != NULL){
            legion_parse_args(cliArgs, out);
        } else {
            return;
        }
        // legion_quit(daemonNodeHead);
    }

    // TODO  Free daemon structs  TODO 
    
    struct daemonNode *tempNode = NULL;
    while(daemonNodeHead != NULL){
        tempNode = daemonNodeHead;
        daemonNodeHead = daemonNodeHead->nextDaemon;
        free(tempNode);
    }

    // TODO  Put sf_fini()
    
}

/*  SECTION  Parsing args functions */

/*  SPECIFICATION  Single quote characters (') are treated specially: when such a character is encountered, all the subsequent characters up to the next single quote (or the end of the line, if there
is no other single quote character) are treated as a single field, regardless of whether any of those characters are spaces.  The quote characters themselves do not become part of the field. */

int legion_parse_args(char curStr[], FILE *out){
    char *curArg;
    char *wholeStr = curStr;
    char *token;
    char *beginOfArgs = NULL;
    argArrIdx = 0;
    int curIdx = 0;
    bool moreThanFourArgs = false;
    argsArrayLen = 0;

    while((int)wholeStr[curIdx] != 10){
        argsArrayLen++;
        curIdx++;
    }
    curArg = malloc((size_t)(argsArrayLen * 2));
    beginOfArgs = curArg;
    curIdx = 0;

    while((int)wholeStr[curIdx] != 10){
        if((int)wholeStr[curIdx] == 60){
            curArg = beginOfArgs;
            free(curArg);
            return EXIT_SUCCESS;
        }
        if((int)wholeStr[curIdx] == 39){
            curIdx++;
            while((int)wholeStr[curIdx] != 10 && (int)wholeStr[curIdx] != 39){
                *curArg = wholeStr[curIdx];
                curArg++;
                curIdx++;
            }
            *curArg = '@';
            curArg++;
            while((int)wholeStr[curIdx] == 39){
                curIdx++;
            }
            while(wholeStr[curIdx] != 10 && wholeStr[curIdx] == 32){ // Skip extra spaces
                curIdx++;
            }
        }
        else{
            while(wholeStr[curIdx] != 10 && wholeStr[curIdx] != 32){
                *curArg = wholeStr[curIdx];
                curArg++;
                curIdx++;
            }
            *curArg = '@';
            curArg++;
            while(wholeStr[curIdx] != 10 && wholeStr[curIdx] == 32){ // Skip extra spaces
                curIdx++;
            }
            if(wholeStr[curIdx] == 10){
                break;
            }
        }
    }
    curArg = beginOfArgs;
    char argContainer[100];
    while ((token = strtok_r(curArg, "@", &curArg))){
        if(!moreThanFourArgs && argArrIdx == 3){
            strcpy(argContainer, token);
            argArr[argArrIdx] = argContainer;
            moreThanFourArgs = true;
        }
        else if(moreThanFourArgs){
            strcat(argContainer, " ");
            strcat(argContainer, token);
            argArr[argArrIdx] = argContainer;
        }
        else {
            argArr[argArrIdx++] = token;
        }

    } 

    curArg = beginOfArgs;

    legion_check_args(out);
    free(curArg);
    return EXIT_SUCCESS;
}

void legion_check_args(FILE *out){
    if(argArrIdx > 0){
        if(strcmp(argArr[0], "help") == 0){
            HELP_MSG(out);
        }
        else if(strcmp(argArr[0], "quit") == 0){
            legion_quit();
        }
        else if(strcmp(argArr[0], "register") == 0){
            // sleep(10);
            if((argArrIdx - 1) < 2){
                fprintf(out, "%s \n", "Usage: register <daemon> <cmd-and-args>");
                sf_error("command execution");
                fprintf(out, "Error executing command: %s \n", argArr[0]);
                fflush(out);
            }
            else if(legion_daemon_name_exists(argArr[1]) == NULL){
                legion_register(argArr[1], argArr[2], argArrIdx - 3);
            }
            else{
                fprintf(out, "Daemon %s is already registered.\n", argArr[1]);
                sf_error("command execution");
                fprintf(out, "Error executing command: %s \n", argArr[0]);
                fflush(out);
            }
        }
        else if(strcmp(argArr[0], "unregister") == 0){
            if((argArrIdx - 1) > 1){
                legion_check_args_print_err(out, argArrIdx, 2, "unregister");
            }
            else {
                if(!legion_unregister(argArr[1])){
                    fprintf(out, "Daemon %s is not registered.\n", argArr[1]);
                    sf_error("command execution");
                    fprintf(out, "Error executing command: %s \n", argArr[0]);
                    fflush(out);
                }
            }
        }
        else if(strcmp(argArr[0], "status") == 0){
            if((argArrIdx - 1) > 1){
                legion_check_args_print_err(out, argArrIdx, 1, "status");
            }
            else {
                struct daemonNode *curNode = legion_status(argArr[1]);
                fprintf(out, "%s\t%i\t%s\n", curNode->daemonName, curNode->daemonProcessID, daemon_status_map[curNode->daemonStatus]);
                fflush(out);
            }
        }
        else if(strcmp(argArr[0], "status-all") == 0){
            if((argArrIdx - 1) > 0){
                legion_check_args_print_err(out, argArrIdx, 0, "status-all");
            }
            else {
                legion_status_all(out);
            }
        }
        else if(strcmp(argArr[0], "start") == 0){
            if((argArrIdx - 1) > 1){
                legion_check_args_print_err(out, argArrIdx, 1, "start");
            }
            // else {
            //     // print arr struct out
            // }
        }
        else if(strcmp(argArr[0], "stop") == 0){
            if((argArrIdx - 1) > 1){
                legion_check_args_print_err(out, argArrIdx, 1, "stop");
            }
            // else {
            //     // print arr struct out
            // }
        }
        else if(strcmp(argArr[0], "logrotate") == 0){
            if((argArrIdx - 1) > 1){
                legion_check_args_print_err(out, argArrIdx, 1, "logrotate");
            }
            // else {
            //     // print arr struct out
            // }
        }
        else {
            fprintf(out, "Unrecognized command: %s \n", argArr[0]);
            sf_error("command execution");
            fprintf(out, "Error executing command: %s \n", argArr[0]);
            fflush(out);
        }
    }
    return;
}

struct daemonNode* legion_daemon_name_exists(char *curName){
    struct daemonNode *runnerNode = NULL;
    runnerNode = daemonNodeHead;
    while(runnerNode->nextDaemon != NULL){
        runnerNode = runnerNode->nextDaemon;
        if(strcmp(runnerNode->daemonName, curName) == 0){
            return runnerNode;
        }
    }
    return NULL;
}

void legion_check_args_print_err(FILE *out, int givenArgCnt, int requiredArgCnt, char *curArg){
    fprintf(out, "Wrong number of args (given: %i, required: %i) for command 'status-all' \n", givenArgCnt - 1, requiredArgCnt);
    sf_error("command execution");
    fprintf(out, "Error executing command: %s \n", curArg);
    fflush(out);
    return;
}

void legion_init(){
    return;
}

/*  SECTION  Arg option functions */

void legion_quit(){
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

void legion_register(char *curName, char *curExe, int argCnt){
    struct daemonNode *tempNode = (struct daemonNode*) malloc(sizeof(struct daemonNode));
    strncpy(tempNode->daemonName, curName, DAEMON_NAME_MAX_LENGTH);
    strncpy(tempNode->daemonExe, curExe, DAEMON_EXE_MAX_LENGTH);
    tempNode->numberOfArgs = argCnt;
    tempNode->daemonStatus = status_inactive;
    tempNode->daemonProcessID = 0;
    tempNode->nextDaemon = NULL;

    if(daemonNodeHead->nextDaemon == NULL){
        daemonNodeHead->nextDaemon = tempNode;
    }
    else{
        tempNode->nextDaemon = daemonNodeHead->nextDaemon;
        daemonNodeHead->nextDaemon = tempNode;
    }

    if((argArrIdx - 3) > 0){
        sf_register(tempNode->daemonName, argArr[3]);
    }
    else{
        sf_register(tempNode->daemonName, "");
    }

    return;
}

int legion_unregister(char* curName){
    if(daemonNodeHead->nextDaemon == NULL){
        return 0;
    }
    struct daemonNode *foundNode = NULL;
    struct daemonNode *runnerNode = NULL;
    runnerNode = daemonNodeHead;
    while(runnerNode->nextDaemon != NULL){
        if(strcmp(runnerNode->nextDaemon->daemonName, curName) == 0){
            if(runnerNode->nextDaemon->daemonStatus == status_inactive){
                foundNode = runnerNode->nextDaemon;
                sf_unregister(foundNode->daemonName);
                runnerNode->nextDaemon = foundNode->nextDaemon;
                free(runnerNode->nextDaemon);
                return 1;
            }
            else {
                return 0;
            }
        }
        runnerNode = runnerNode->nextDaemon;
    }
    return 0;
}

struct daemonNode* legion_status(char* curName){
    struct daemonNode *runnerNode = NULL;
    runnerNode = daemonNodeHead;
    while(runnerNode->nextDaemon != NULL){
        runnerNode = runnerNode->nextDaemon;
        if(strcmp(runnerNode->daemonName, curName) == 0){
            return runnerNode;
        }
    }
    return NULL;
}

void legion_status_all(FILE *out){
    struct daemonNode *runnerNode = NULL;
    runnerNode = daemonNodeHead;
    while(runnerNode->nextDaemon != NULL){
        runnerNode = runnerNode->nextDaemon;
        fprintf(out, "%s\t%i\t%s\n", runnerNode->daemonName, runnerNode->daemonProcessID, daemon_status_map[runnerNode->daemonStatus]);
        fflush(out);
    }
    return;
}

void legion_start(char* curName){
    return;
}

int legion_stop(char* curName){
    struct daemonNode *curNode = legion_daemon_name_exists(curName);
    char errMsg[100];

    if(curNode == NULL){
        sprintf(errMsg, "Daemon %s is not registered.\n", curName);
        sf_error(errMsg);
        return STOP_ERROR;
    }

    if(curNode->daemonStatus == status_exited || curNode->daemonStatus == status_crashed){
        curNode->daemonStatus = status_inactive;
        sf_reset(curNode->daemonName);
        return STOP_SUCCESS;
    }

    if(curNode->daemonStatus != status_active){
        sprintf(errMsg, "Daemon %s is not active.\n", curName);
        sf_error(errMsg);
        return STOP_ERROR;
    }

    curNode->daemonStatus = status_stopping;
    sf_stop(curNode->daemonName, curNode->daemonProcessID);
    if(kill(curNode->daemonProcessID, SIGTERM) != 0){
        sprintf(errMsg, "Unable to send SIGTERM to daemon %s.\n", curName);
        sf_error(errMsg);
        return STOP_ERROR;
    }

    sigset_t curMask;
    sigemptyset(&curMask);
    alarm(CHILD_TIMEOUT);
    sigsuspend(&curMask);

    if(sigchld_flag){
        alarm(0); // Get rid of past alarm
        pid_t curPID = waitpid(curNode->daemonProcessID, NULL, 0);
        if(curPID == -1){
            sprintf(errMsg, "WaitPID error with daemon %s.\n", curName);
            sf_error(errMsg);
            sf_crash(curNode->daemonName, curPID, SIGTERM);
            return STOP_ERROR;
        }
        sigchld_flag = false;
        sf_term(curNode->daemonName, curPID, 0);
        return STOP_SUCCESS;
    }

    if(sigalrm_flag){
        kill(curNode->daemonProcessID, SIGKILL);
        sprintf(errMsg, "Daemon %s killed with SIGKILL.\n", curName);
        sf_error(errMsg);
        return STOP_ERROR;
    }

    return STOP_ERROR;

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

/*  SECTION  Signal Handlers */

void sigint_handler(int sig) /* Safe SIGINT handler */
{
    running = false;
}

void sigchld_handler(int sig) /* Safe SIGINT handler */
{
    sigchld_flag = true;
}

void sigalrm_handler(int sig) /* Safe SIGINT handler */
{
    sigalrm_flag = true;
}