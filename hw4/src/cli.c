/*
 * Legion: Command-line interface
 */

#include "legion.h"
#include "cli.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <stdbool.h>

void run_cli(FILE *in, FILE *out)
{
    pid_t cpid;
    if(fork() == 0){
        exit(0);
    }
    else {
        cpid = wait(NULL);
    }
    
    struct daemonNode *head = (struct daemonNode*) malloc(sizeof(struct daemonNode));

    while(1){
        char name[100];
        printf("legion> ");
        fgets(name, 10, stdin);
        printf("Hello %s ! %d \n", name, getpid());
        printf("WELL WELL %d\n", cpid);
    }
}

void legion_init(){
    return;
}

void legion_quit(){
    // TODO: Remove all daemons from processes
    // TODO: Clean up in general
    sf_fini();
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