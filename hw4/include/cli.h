#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


#define DAEMON_NAME_MAX_LENGTH 100
#define CUR_ARG_MAX_LENGTH 100
#define CLI_ARGs_MAX_LENGTH 10000
#define ARGS_ARRAY_LENGTH 1000

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

struct argNode {
    char curArg[CUR_ARG_MAX_LENGTH];
    struct argNode *nextArg;
};

struct daemonNode {
    char daemonName[DAEMON_NAME_MAX_LENGTH];
    int daemonStatus;
    int daemonProcessID;
    struct argNode *argHead;
    struct daemonNode *nextDaemon;
};


const char *daemon_status_map[7] = {"status_unknown", "status_inactive", "status_starting", "status_active", "status_stopping", "status_exited", "status_crashed"};


void legion_parse_args(char curStr[], FILE *out, struct daemonNode *daemonNodeHead);
int legion_char_idx(char curStr[]);
void legion_init();
void legion_quit(struct daemonNode* daemonNodeHead);
void legion_free_daemon_node_head(struct daemonNode *daemonNodeHead);
void legion_help();
void legion_register(char* curDaemon);
void legion_unregister(char* daemonName, char* daemonExec);
void legion_status(char* curDaemon);
void legion_status_all();
void legion_start(char* daemonName);
void legion_stop(char* daemonName);
void legion_logrotate(char* curDaemon);

/* Helper functions to take out before submitting */
void print_word_chars(char* curWord);

// NOTE  Always use fflush(out) after priting out stuff to cli

// NOTE  sf_init(void) - To be called when legion begins execution.  A call to this function
// has already been included for you in main().

// NOTE  sf_fini(void) - To be called when legion terminates, either as a result of the
// user having entered the quit command or as a result of the delivery of a SIGINT
// signal.  A call to this function has already been included for you in main(),
// but if you terminate the program in some way other than by returning from main,
// you will have to include additional call(s).

// NOTE  sf_register(char *daemon_name) - To be called when a daemon is registered.

// NOTE  sf_unregister(char *daemon_name) - To be called when a daemon is unregistered.

// NOTE  sf_start(char *daemon_name) - To be called when a daemon is about to be started.

// NOTE  sf_active(char *daemon_name, pid_t pid) - To be called when a daemon process
// has been successfully forked, the synchronization message has been received,
// and the daemon has been placed into the active state.

// NOTE  sf_stop(char *daemon_name, pid_t pid) - To be called when legion has sent a daemon
// process a SIGTERM signal and has set it to the stopping state.

// NOTE  sf_kill(char *daemon_name, pid_t pid) - To be called when legion has sent a daemon
// process a SIGKILL signal, after a previously sent SIGTERM did not cause the daemon
// to terminate in a timely fashion.

// NOTE  sf_term(char *daemon_name, pid_t pid, int exit_status) - To be called when legion
// learns (via receipt of SIGCHLD and subsequent call to waitpid(2)) that a daemon
// process has terminated via exit().

// NOTE  sf_crash(char *daemon_name, pid_t pid, int signal) - To be called when legion
// learns (via receipt of SIGCHLD and subsequent call to waitpid(2)) that a daemon
// process has terminated abnormally as a result of a signal.

// NOTE  sf_reset(char *daemon_name) - To be called when a daemon that has previously exited
// or crashed is reset (as a result of a stop command entered by the user) to the inactive
// state.

// NOTE  sf_logrotate(char *daemon_name) - To be called when legion performs the log rotation
// procedure for a daemon, before the daemon is restarted.

// NOTE  sf_error(char *reason) - To be called when the execution of a user command fails due
// to an error.  A string describing the error may be passed, or NULL.

/*
 TODO  help - print out help message

 TODO  quit - free structs, quit program

 TODO  register - read from arg, create struct

 TODO  unregister - free struct

 TODO  status - show status of daemon struct

 TODO  status-all - show status of all daemon structs

 TODO  start - start daemon using struct, change struct

 TODO  stop - stop daemon using struct, change struct

 TODO  logrotate - change file directory of process 
 */