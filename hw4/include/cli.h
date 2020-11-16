#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


#define DAEMON_NAME_MAX_LENGTH 100
#define DAEMON_ARGS_MAX_LENGTH 100
#define DAEMON_EXE_MAX_LENGTH 100
#define CLI_ARGS_MALLOC 1000
#define CUR_ARG_MAX_LENGTH 1000
#define LOGFILE_PATH_LENGTH 100
#define SNPRINTF_MAX_LEN 1000

#define LEGION_ERROR 0
#define LEGION_SUCCESS 1

#define HELP_MSG(out) do { \
fprintf(out, "Available commands:\n" \
"help (0 args) Print this help message\n" \
"quit (0 args) Quit the program\n" \
"register (0 args) Register a daemon\n" \
"unregister (1 args) Unregister a daemon\n" \
"status (1 args) Show the status of a daemon\n" \
"status-all (0 args) Show the status of all daemons\n" \
"start (1 args) Start a daemon\n" \
"stop (1 args) Stop a daemon\n" \
"logrotate (1 args) Rotate log files for a daemon\n" \
); \
return; \
} while(0)

struct daemonNode {
    char daemonName[DAEMON_NAME_MAX_LENGTH];
    char daemonExe[DAEMON_EXE_MAX_LENGTH];
    char daemonArgs[DAEMON_ARGS_MAX_LENGTH];
    int daemonStatus;
    pid_t daemonProcessID;
    struct daemonNode *nextDaemon;
};


const char *daemon_status_map[7] = {"unknown", "inactive", "starting", "active", "stopping", "exited", "crashed"};


/*  SECTION  Parsing args functions */
int legion_parse_args(char *curStr, FILE *out);
void legion_check_args(FILE *out);
struct daemonNode* legion_daemon_name_exists(char *curName);
void legion_check_args_print_err(FILE *out, int givenArgCnt, int requiredArgCnt, char *curArg);


/*  SECTION  Arg option functions */
void legion_quit();
void legion_help();
int legion_register(char *curName, char *curExe, char *args);
int legion_unregister(char* curName);
struct daemonNode* legion_status(char* curName);
void legion_status_all(FILE *out);
int legion_start(char* curName, char logFileVersion);
int legion_stop(char* curName);
int legion_logrotate(char* curName);

/*  SECTION  Signal Handlers */
void sigint_handler(int sig);
void sigtstp_handler(int sig);
void sigchld_handler(int sig);
void sigalrm_handler(int sig);

/*  SECTION Helper functions to take out before submitting */

// NOTE  Always use fflush(out) after priting out stuff to cli

// NOTE  sf_term(char *daemon_name, pid_t pid, int exit_status) - To be called when legion
// learns (via receipt of SIGCHLD and subsequent call to waitpid(2)) that a daemon
// process has terminated via exit().  NOTE 

// NOTE  sf_crash(char *daemon_name, pid_t pid, int signal) - To be called when legion
// learns (via receipt of SIGCHLD and subsequent call to waitpid(2)) that a daemon
// process has terminated abnormally as a result of a signal.

// NOTE  sf_logrotate(char *daemon_name) - To be called when legion performs the log rotation
// procedure for a daemon, before the daemon is restarted.


/*
 TODO  start - deal with yoyo
 */