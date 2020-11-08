#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


#define DAEMON_NAME_MAX_LENGTH 100

struct daemonNode {
    char daemonName[DAEMON_NAME_MAX_LENGTH];
    int daemonStatus;
    int daemonProcessID;
    struct daemonNode *nextDaemon;
};


const char *daemon_status_map[7] = {"status_unknown", "status_inactive", "status_starting", "status_active", "status_stopping", "status_exited", "status_crashed"};


void legion_init();
void legion_quit();
void legion_help();
void legion_logrotate(char* curDaemon);
