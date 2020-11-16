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
#include <dirent.h>

bool running = true;
volatile sig_atomic_t sigchld_flag = false;
volatile sig_atomic_t  sigalrm_flag = false;
char *argArr[CUR_ARG_MAX_LENGTH];
int argArrIdx = 0;
int argsArrLen = 0;
struct daemonNode *daemonNodeHead;

void run_cli(FILE *in, FILE *out)
{
    // TODO  Establish all signal handlers before doing anything
    // sf_init();
    struct sigaction sigalrm_action;
    memset(&sigalrm_action, '\0', sizeof(sigalrm_action));
    sigalrm_action.sa_handler = &sigalrm_handler;
    signal(SIGCHLD, sigchld_handler);
    // signal(SIGALRM, sigalrm_handler);
    sigaction(SIGALRM, &sigalrm_action, NULL);
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);

    daemonNodeHead = (struct daemonNode*) malloc(sizeof(struct daemonNode));
    strncpy(daemonNodeHead->daemonName, "daemonNodeHead", DAEMON_NAME_MAX_LENGTH);
    strncpy(daemonNodeHead->daemonExe, "daemonNodeHead", DAEMON_EXE_MAX_LENGTH);
    daemonNodeHead->daemonStatus = -1;
    daemonNodeHead->daemonProcessID = -1;
    daemonNodeHead->nextDaemon = NULL;

    while(running){
        size_t getLineLen = CLI_ARGS_MALLOC;
        char *cliArgs = NULL;
        fprintf(out, "%s", "legion> ");
        fflush(out);
        if(getline(&cliArgs, &getLineLen, in) != -1){
            legion_parse_args(cliArgs, out);
            free(cliArgs);
        } else {
            free(cliArgs);
            return;
        }
    }

    return;
    
}

/*  SECTION  Parsing args functions */

/*  SPECIFICATION  Single quote characters (') are treated specially: when such a character is encountered, all the subsequent characters up to the next single quote (or the end of the line, if there
is no other single quote character) are treated as a single field, regardless of whether any of those characters are spaces.  The quote characters themselves do not become part of the field. */

int legion_parse_args(char *curStr, FILE *out){
    char *curArg;
    char *wholeStr = curStr;
    char *token = "";
    char *beginOfArgs = NULL;
    argArrIdx = 0;
    int curIdx = 0;
    bool moreThanFourArgs = false;
    argsArrLen = 0;

    while((int)wholeStr[curIdx] != 10){
        argsArrLen++;
        curIdx++;
    }
    curArg = calloc((argsArrLen * 2), sizeof(char));
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
    char argContainer[500];
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
    // free(curStr);
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
            if((argArrIdx - 1) < 2){
                fprintf(out, "%s \n", "Usage: register <daemon> <cmd-and-args>");
                sf_error("command execution");
                fprintf(out, "Error executing command: %s \n", argArr[0]);
                fflush(out);
            }
            else if(legion_daemon_name_exists(argArr[1]) == NULL){
                if(argArr[3] != NULL){
                    legion_register(argArr[1], argArr[2], argArr[3]);
                }
                else {
                    legion_register(argArr[1], argArr[2], "");
                }
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
                if(curNode != NULL){
                    fprintf(out, "%s\t%i\t%s\n", curNode->daemonName, curNode->daemonProcessID, daemon_status_map[curNode->daemonStatus]);
                    fflush(out);
                }
                else {
                    sf_error("command execution");
                    fprintf(out, "Error executing command: %s \n", argArr[0]);
                    fflush(out);
                }
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
            else {
                if(legion_start(argArr[1], '0') == LEGION_ERROR){
                    fprintf(out, "Error executing command: %s \n", argArr[0]);
                    fflush(out);
                }
            }
        }
        else if(strcmp(argArr[0], "stop") == 0){
            if((argArrIdx - 1) > 1){
                legion_check_args_print_err(out, argArrIdx, 1, "stop");
            }
            else {
                if(legion_stop(argArr[1]) == LEGION_ERROR){
                    fprintf(out, "Error executing command: %s \n", argArr[0]);
                    fflush(out);
                }
            }
        }
        else if(strcmp(argArr[0], "logrotate") == 0){
            if((argArrIdx - 1) > 1){
                legion_check_args_print_err(out, argArrIdx, 1, "logrotate");
            }
            else {
                if(legion_logrotate(argArr[1]) == LEGION_ERROR){
                    fprintf(out, "Error executing command: %s \n", argArr[0]);
                    fflush(out);
                }
            }
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

/*  SECTION  Arg option functions */

void legion_quit(){
    running = false;
    struct daemonNode *tempNode = daemonNodeHead->nextDaemon;

    // Stop running daemons
    while(tempNode != NULL){
        if(tempNode->daemonStatus == status_active){
            legion_stop(tempNode->daemonName);
        }
        tempNode = tempNode->nextDaemon;
    }

    // Free daemon structs
    tempNode = NULL;
    while(daemonNodeHead != NULL){
        tempNode = daemonNodeHead;
        daemonNodeHead = daemonNodeHead->nextDaemon;
        if(strcmp(tempNode->daemonName, "daemonNodeHead") != 0){
            sf_unregister(tempNode->daemonName);
        }
        free(tempNode);
    }

    sf_fini();
    _exit(0);
}

void legion_help(){
    return;
}

int legion_register(char *curName, char *curExe, char *args){
    struct daemonNode *tempNode = (struct daemonNode*) malloc(sizeof(struct daemonNode));
    strncpy(tempNode->daemonName, curName, DAEMON_NAME_MAX_LENGTH);
    strncpy(tempNode->daemonExe, curExe, DAEMON_EXE_MAX_LENGTH);
    strncpy(tempNode->daemonArgs, args, DAEMON_ARGS_MAX_LENGTH);
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

    return LEGION_SUCCESS;

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
                runnerNode->nextDaemon = foundNode->nextDaemon;
                sf_unregister(foundNode->daemonName);
                free(foundNode);
                return LEGION_SUCCESS;
            }
            else {
                return LEGION_ERROR;
            }
        }
        runnerNode = runnerNode->nextDaemon;
    }
    return LEGION_ERROR;
}

struct daemonNode* legion_status(char* curName){
    return legion_daemon_name_exists(curName);
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

int legion_start(char* curName, char logFileVersion){
    // char errMsg[100];
    int pipefd[2];
    pid_t childPID;
    DIR *logDir;
    struct daemonNode *curNode = legion_daemon_name_exists(curName);
    if((curNode == NULL) || (curNode->daemonStatus != status_inactive)){
        sf_error("command execution");
        return LEGION_ERROR;
    }

    logDir = opendir(LOGFILE_DIR);
    if(logDir == NULL){
        mkdir(LOGFILE_DIR, 0777);
    }
    else {
        free(logDir);
    }

    curNode->daemonStatus = status_starting;

    if(pipe(pipefd) == -1){
        curNode->daemonStatus = status_inactive;
        sf_error("Pipe error");
        return LEGION_ERROR;
    }

    childPID = fork();
    if(childPID == -1){
        curNode->daemonStatus = status_inactive;
        sf_error("Fork error");
        return LEGION_ERROR;
    }
    
    if(childPID == 0){ /* Child */ 
        close(pipefd[0]); /* Close child read side */
        char *logFilePath = malloc(strlen(LOGFILE_DIR) + strlen(curName) + 2);
        char *envPath = malloc(strlen(DAEMONS_DIR) + strlen(getenv(PATH_ENV_VAR) + 2));
        if(getenv(PATH_ENV_VAR) == NULL){
            sf_error("Child has no PATH");
            return LEGION_ERROR;
        }
        if(envPath != NULL){
            snprintf(envPath, SNPRINTF_MAX_LEN, "%s:%s", DAEMONS_DIR, getenv(PATH_ENV_VAR));
        }
 
        if(logFilePath != NULL){
            snprintf(logFilePath, SNPRINTF_MAX_LEN, "%s/%s.log.%c", LOGFILE_DIR, curName, logFileVersion);
        }
        setenv(PATH_ENV_VAR, envPath, 1);
        freopen(logFilePath, "a", stdout);

        if(dup2(pipefd[1], SYNC_FD) == -1){ /* Redirecting write side to SYNC_FD */
            sf_error("dup2 error with child");
            return LEGION_ERROR;
        }

        if(setpgid(getpid(), 0)){ /* New process group */
            sf_error("setpgid error with child");
            return LEGION_ERROR;
        }

        char *execEnv[2] = {getenv(PATH_ENV_VAR), NULL};
        char *argv[2] = {curNode->daemonArgs, NULL};
        if(execvpe(curNode->daemonExe, argv, execEnv) == -1){
            sf_error("Error executing daemon");
            return LEGION_ERROR;
        }

        close(pipefd[1]); /* Close child write side */
        close(SYNC_FD);
        free(logFilePath);
        free(envPath);
        exit(0);

    }
    else{ /* Parent */
        close(pipefd[1]);
        sigset_t curMask;
        sigemptyset(&curMask);
        sigaddset(&curMask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &curMask, NULL);
        char *buf = calloc(1, sizeof(char));
        
        alarm(CHILD_TIMEOUT);
        
        if(read(pipefd[0], buf, 1)){
            alarm(0);
            sf_start(curNode->daemonName);
            curNode->daemonProcessID = childPID;
            curNode->daemonStatus = status_active;
            sf_active(curNode->daemonName, curNode->daemonProcessID);
        }

        if(sigalrm_flag){
            sf_stop(curName, childPID);
            curNode->daemonStatus = status_crashed;
            free(buf);
            sigalrm_flag = false;
            return LEGION_ERROR;
        }
        close(pipefd[0]);
        free(buf);
        // Read byte
        // Set node to active
    }
    return LEGION_SUCCESS;
}

int legion_stop(char* curName){
    struct daemonNode *curNode = legion_daemon_name_exists(curName);
    char errMsg[100];

    if(curNode->daemonStatus == status_exited || curNode->daemonStatus == status_crashed){
        curNode->daemonStatus = status_inactive;
        sf_reset(curNode->daemonName);
        return LEGION_SUCCESS;
    }

    if(curNode == NULL || curNode->daemonStatus != status_active){
        sf_error("command execution");
        return LEGION_ERROR;
    }


    curNode->daemonStatus = status_stopping;
    sf_stop(curNode->daemonName, curNode->daemonProcessID);
    if(kill(curNode->daemonProcessID, SIGTERM) != 0){
        sprintf(errMsg, "Unable to send SIGTERM to daemon %s.", curName);
        sf_error(errMsg);
        return LEGION_ERROR;
    }

    sigset_t curMask;
    sigemptyset(&curMask);
    alarm(CHILD_TIMEOUT);
    sigsuspend(&curMask);

    if(sigchld_flag){
        alarm(0); // Get rid of past alarm
        int status = 0;
        pid_t curPID = waitpid(curNode->daemonProcessID, &status, 0);
        if(curPID == -1){
            sprintf(errMsg, "WaitPID error with daemon %s.", curName);
            sf_error(errMsg);
            curNode->daemonProcessID = 0;
            curNode->daemonStatus = status_crashed;
            sf_crash(curNode->daemonName, curPID, WTERMSIG(status));
            return LEGION_ERROR;
        }
        curNode->daemonStatus = status_exited;
        sigchld_flag = false;
        sf_term(curNode->daemonName, curPID, 0);
        return LEGION_SUCCESS;
    }

    if(sigalrm_flag){
        sf_kill(curNode->daemonName, curNode->daemonProcessID);
        kill(curNode->daemonProcessID, SIGKILL);
        sprintf(errMsg, "Daemon %s killed with SIGKILL.", curName);
        sf_error(errMsg);
        sigalrm_flag = false;
        return LEGION_ERROR;
    }

    curNode->daemonStatus = status_exited;
    return LEGION_SUCCESS;

}

int legion_logrotate(char* curName){
    struct daemonNode* curNode = legion_daemon_name_exists(curName);
    char *logFilePath = calloc(LOGFILE_PATH_LENGTH, sizeof(char));
    if(curNode == NULL){
        sf_error("command execution");
        return LEGION_ERROR;
    }

    if(curNode->daemonStatus != status_active){
        sf_error("Daemon not started");
        return LEGION_ERROR;
    }

    if(opendir(LOGFILE_DIR) == NULL){
        mkdir(LOGFILE_DIR, 0777);
    }

    /* Check if max files reached, delete version 7, up all other 6 files */
    if(logFilePath != NULL){
        sprintf(logFilePath, "%s/%s.log.%c", LOGFILE_DIR, curName, LOG_VERSIONS - 1 + '0');
    }

    if(access(logFilePath, F_OK) == 0){
        char *oldLogFilePath = calloc(LOGFILE_PATH_LENGTH, sizeof(char));
        char *newLogFilePath = calloc(LOGFILE_PATH_LENGTH, sizeof(char));
        unlink(logFilePath); // Delete version 6 log file
        int i = LOG_VERSIONS - 2;
        while(i >= 0){
            sprintf(oldLogFilePath, "%s/%s.log.%c", LOGFILE_DIR, curName, i + '0');
            sprintf(newLogFilePath, "%s/%s.log.%c", LOGFILE_DIR, curName, i + 1 + '0');
            if(rename(oldLogFilePath, newLogFilePath) == -1){
                sf_error("Unable to logrotate files");
                return LEGION_ERROR;
            }
            i -= 1;
        }
        free(oldLogFilePath);
        free(newLogFilePath);
        sf_logrotate(curName);
        legion_stop(curName);
        legion_stop(curName);
        legion_start(curName, '0');
    }
    /* Not max files reached, create next file and redirect daemon write to it*/
    else{ 
        int lastVersion = 0;
        sprintf(logFilePath, "%s/%s.log.%c", LOGFILE_DIR, curName, lastVersion + '0');
        while(access(logFilePath, F_OK) == 0){
            lastVersion += 1;
            sprintf(logFilePath, "%s/%s.log.%c", LOGFILE_DIR, curName, lastVersion + '0');
        }
        sprintf(logFilePath, "%s/%s.log.%c", LOGFILE_DIR, curName, lastVersion + '0');
        sf_logrotate(curName);
        legion_stop(curName);
        legion_stop(curName);
        legion_start(curName, lastVersion + '0');
    }

    free(logFilePath);
    return LEGION_SUCCESS;
}

/*  SECTION  Signal Handlers */

void sigint_handler(int sig){
    legion_quit();
}

void sigtstp_handler(int sig){
    legion_quit();
}

void sigchld_handler(int sig){
    sigchld_flag = true;
}

void sigalrm_handler(int sig){
    sigalrm_flag = true;
}