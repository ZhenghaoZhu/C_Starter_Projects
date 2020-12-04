#include <stdlib.h>
#include <stdio.h>
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

#include <sys/signal.h>
#include "csapp.h"



#ifdef DEBUG
int _debug_packets_ = 1;
#endif

static void terminate(int status);

void sighub_handler(int sig) {
    terminate(EXIT_SUCCESS);
}

/*
 * "Jeux" game server.
 *
 * Usage: jeux <port>
 */
int main(int argc, char* argv[]){
    if(argc < 3){
        fprintf(stderr, "usage: -p <port>\n");
    }
    struct sigaction act;
    act.sa_handler = sighub_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction (SIGHUP, &act, NULL) < 0) {
        perror ("sigaction");
        exit (-1);
    }

    client_registry = creg_init();
    player_registry = preg_init();

    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    listenfd = Open_listenfd(argv[2]);
    debug("%li: Jeux server listening on port %s", pthread_self(), argv[2]);
    while (1) {
        clientlen=sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        Pthread_create(&tid, NULL, jeux_client_service, connfdp);
    }
    terminate(EXIT_FAILURE);
}

/*  TODO 
 *
 * - Task 6 (Client Module [client.c])
 * - Task 9 (Game Module [game.c])
 * 
 */

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);
    
    debug("%ld: Waiting for service threads to terminate...", pthread_self());
    creg_wait_for_empty(client_registry);
    debug("%ld: All service threads terminated.", pthread_self());

    // Finalize modules.
    creg_fini(client_registry);
    preg_fini(player_registry);

    debug("%ld: Jeux server terminating", pthread_self());
    debug("EXIT STATUS: %i", status);
    exit(status);
}



