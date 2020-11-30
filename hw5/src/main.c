#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "debug.h"
#include "protocol.h"
#include "server.h"
#include "client_registry.h"
#include "player_registry.h"
#include "jeux_globals.h"
#include "jeux_helper.h"
#include "csapp.h"

#ifdef DEBUG
int _debug_packets_ = 1;
#endif

volatile sig_atomic_t sighup_caught = 0;

static void terminate(int status);

/*
 * "Jeux" game server.
 *
 * Usage: jeux <port>
 */
int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.

    if((argc != 3) || (strcmp(argv[1], "-p") != 0)){
        fprintf(stdout, "Usage: demo/jeux -p <port> \n");
        return -1;
    }

    struct sigaction sa;
    sa.sa_handler = &handle_SIGHUP;
    sigfillset(&sa.sa_mask);
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        perror("SIGHUP handler setup error.");
    }

    // Perform required initializations of the client_registry and
    // player_registry.
    client_registry = creg_init();
    player_registry = preg_init();

    //  TODO  Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function jeux_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    /*  NOTE   It is the caller's responsibility to free() the payload once it is no longer needed.*/
    
    /*  NOTE  Remember that it is always possible for read() and write() to read or write fewer bytes than requested.  You must check for and handle these "short count" situations. */

    int listenfd, *connfdp;
    socklen_t clientLen;
    struct sockaddr_storage clientAddr;
    pthread_t tid;

    listenfd = Open_listenfd(argv[2]);
    debug("%li: Jeux server listening on port %s", pthread_self(), argv[2]);


    while(true){
        clientLen = sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientAddr, &clientLen);
        Pthread_create(&tid, NULL, mainListeningThread, connfdp);
        if (sighup_caught){
            sighup_caught = 0;
            terminate(EXIT_SUCCESS);
        }
    }
    
    /* fprintf(stderr, "You have to finish implementing main() "
	    "before the Jeux server will function.\n"); */

    terminate(EXIT_SUCCESS);
}

/* 
 * Function called to create a thread running jeux_client_service()
 */
void* mainListeningThread(void *vargp){
    int connfd = *((int *) vargp);
    Pthread_detach(pthread_self());
    jeux_client_service(vargp);
    Close(connfd);
    return NULL;
}

void handle_SIGHUP(int signum){
    sighup_caught = 1;
    sio_puts("DEFINITELY HERE! \n");
    terminate(EXIT_SUCCESS);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    debug("STATUS %i", status);
    creg_shutdown_all(client_registry);
    
    debug("%ld: Waiting for service threads to terminate...", pthread_self());
    creg_wait_for_empty(client_registry);
    debug("%ld: All service threads terminated.", pthread_self());

    // Finalize modules.
    creg_fini(client_registry);
    preg_fini(player_registry);

    debug("%ld: Jeux server terminating", pthread_self());
    exit(status);
}
