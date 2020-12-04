    #include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>

#include "client_registry.h"
#include "semaphore.h"
#include "csapp.h"
#include "jeux_helper.h"
#include "debug.h"
#include "client.h"

struct client_registry_node {
    CLIENT *curClient;
    struct client_registry_node *nextClient;  
};
typedef struct client_registry {
    struct client_registry_node *head;
    int clientCount;
    sem_t registryLock;
    sem_t notEmpty;
} CLIENT_REGISTRY;

CLIENT_REGISTRY* cr_head;

CLIENT_REGISTRY *creg_init(){
    cr_head = (CLIENT_REGISTRY*) Malloc(sizeof(CLIENT_REGISTRY));
    if(cr_head == NULL){
        return NULL;
    }
    cr_head->head = NULL;
    cr_head->clientCount = 0;
    if(sem_init(&(cr_head->registryLock), 0, 1) < 0){
        return NULL;
    }
    if(sem_init(&(cr_head->notEmpty), 0, 1) < 0){
        return NULL;
    }
    debug("%li: Initialize client registry", pthread_self());
    return cr_head;
}

void creg_fini(CLIENT_REGISTRY *cr){
    struct client_registry_node* tempNode;
    P(&(cr->registryLock));
    while(cr->head != NULL){ // Free all nodes
        tempNode = cr->head;
        cr->head = cr->head->nextClient;
        // free(tempNode->curClient);
        // free(tempNode->nextClient);
        free(tempNode);
    }
    V(&(cr->registryLock));
    free(cr->head); // Free head itself
    free(cr); // Free registry
    return;
}

CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd){
    debug("Register client fd %i", fd);
    CLIENT* newClient = client_create(cr, fd);
    struct client_registry_node* newClientNode = (struct client_registry_node*) Malloc(sizeof(struct client_registry_node));
    newClientNode->curClient = newClient;
    newClientNode->nextClient = NULL;
    P(&(cr->registryLock));
    if(cr->clientCount >= MAX_CLIENTS){
        free(newClient);
        free(newClientNode);
        debug("MAX AMOUNT OF CLIENTS REACHED: %i", MAX_CLIENTS);
        P(&(cr->notEmpty));
        V(&(cr->registryLock));
        return NULL;
    }
    struct client_registry_node* tempNode = cr->head;
    if(cr->head == NULL){ // Empty List
        cr->head = newClientNode;
        cr->clientCount += 1;
        debug("Registered client %p with fd[%i]", newClient, fd);
        debug("fd of head client %i", client_get_fd(cr->head->curClient));
        P(&(cr->notEmpty));
        V(&(cr->registryLock));
        return newClient;
    }
    while(tempNode->nextClient != NULL){
        tempNode = tempNode->nextClient;
    }
    tempNode->nextClient = newClientNode;
    debug("Registered client %p with fd[%i]", newClient, fd);
    debug("fd of head client %i", client_get_fd(cr->head->curClient));
    cr->clientCount += 1;
    V(&(cr->registryLock));
    return newClient;
}

int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client){
    // Compare fd's
    debug("Trying to unregister %p from registry %p", client, cr);
    P(&(cr->registryLock));
    struct client_registry_node* tempNode = cr->head;
    struct client_registry_node* prevNode = NULL;
    int crFd = 0;
    int clientFd = 0;
    client_unref(client, "Unregistering client from registry");
    if(cr->head != NULL && cr->head->curClient != NULL){
        crFd = client_get_fd(cr->head->curClient);
        clientFd = client_get_fd(client);
        if(crFd == clientFd){
            cr->head = cr->head->nextClient;
            cr->clientCount -= 1;
            free(tempNode->curClient);
            free(tempNode);
            V(&(cr->registryLock));
            debug("Successfully unregistered %p from registry %p", client, cr);
            return 0;
        }
    }
    while(tempNode->curClient != NULL){
        crFd = client_get_fd(tempNode->curClient);
        if(crFd == clientFd){
            prevNode->nextClient = tempNode->nextClient;
            cr->clientCount -= 1;
            free(tempNode->curClient);
            free(tempNode);
            V(&(cr->registryLock));
            debug("Successfully unregistered %p from registry %p", client, cr);
            return 0;
        }
        prevNode = tempNode;
        tempNode = tempNode->nextClient;
    }
    debug("%p not found in %p", client, cr);
    V(&(cr->registryLock));
    return -1;
}

CLIENT *creg_lookup(CLIENT_REGISTRY *cr, char *user){
    P(&(cr->registryLock));
    debug("Entering creg_lookup");
    struct client_registry_node* tempNode = cr->head;
    if(cr->head == NULL || cr->head->curClient == NULL){
        debug("Exiting creg_lookup");
        return NULL;
    }
    PLAYER* tempPlayer = client_get_player(cr->head->curClient);
    if(strcmp(player_get_name(tempPlayer), user) == 0){ // Check first node
        V(&(cr->registryLock));
        debug("Exiting creg_lookup");
        return cr->head->curClient;
    }
    while(tempNode->curClient != NULL){ // Check middle nodes
        tempPlayer = client_get_player(tempNode->curClient);
        if(strcmp(player_get_name(tempPlayer), user) == 0){
            V(&(cr->registryLock));
            debug("Exiting creg_lookup");
            return tempNode->curClient;
        }
        tempNode = tempNode->nextClient;
    }
    V(&(cr->registryLock));
    debug("Exiting creg_lookup");
    return NULL;
}

PLAYER **creg_all_players(CLIENT_REGISTRY *cr){
    debug("Entering creg_all_players");
    P(&(cr->registryLock));
    PLAYER** player_array = Malloc((cr->clientCount + 1) * sizeof(PLAYER*));
    if(cr->head == NULL){
        V(&(cr->registryLock));
        debug("Exiting creg_all_players");
        return player_array;
    }
    struct client_registry_node* tempNode = cr->head;
    int count = 0;
    debug("IN ALL PLAYERS %p", cr->head->curClient);
    while(tempNode != NULL && tempNode->curClient != NULL){
        player_ref(client_get_player(tempNode->curClient), "Adding to all players array");
        player_array[count] = client_get_player(tempNode->curClient);
        player_unref(client_get_player(tempNode->curClient), "Finished adding to all players array");
        debug("PLAYER %p added to array", tempNode->curClient);
        count += 1;
        tempNode = tempNode->nextClient;
    }
    player_array[cr->clientCount] = NULL;
    V(&(cr->registryLock));
    debug("Exiting creg_all_players");
    return player_array;
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr){
    debug("Entering creg_wait_for_empty");
    // NOTE  creg_shutdown_all should take care of it
    P(&(cr->notEmpty));
    V(&(cr->notEmpty));
    debug("Exiting creg_wait_for_empty");
    return;
}

// Unregister al clients
void creg_shutdown_all(CLIENT_REGISTRY *cr){
    debug("Shutting down all clients in %p", cr);
    P(&(cr->registryLock));
    struct client_registry_node* tempNode = cr->head;
    while(tempNode != NULL){
        shutdown(client_get_fd(tempNode->curClient), SHUT_RD);
        tempNode = tempNode->nextClient;
    }
    cr->clientCount = 0;
    V(&(cr->notEmpty)); // Tell wait_for_empty to continue
    V(&(cr->registryLock));
    debug("Finished shutting down all clients in %p", cr);
    return;
}	