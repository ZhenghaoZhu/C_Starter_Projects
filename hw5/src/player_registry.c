#include <stddef.h>

#include "player_registry.h"
#include "player.h"
#include "semaphore.h"
#include "csapp.h"
#include "debug.h"

struct player_registry_node {
    PLAYER *curPlayer;
    struct player_registry_node *nextPlayer;  
};

typedef struct player_registry {
    struct player_registry_node *head;
    size_t playerCount;
    sem_t registryLock;
} PLAYER_REGISTRY;

PLAYER_REGISTRY* pr_head;

PLAYER_REGISTRY *preg_init(void){
    pr_head = (PLAYER_REGISTRY*) Malloc(sizeof(PLAYER_REGISTRY));
    if(pr_head == NULL){
        return NULL;
    }
    pr_head->head = Malloc(sizeof(struct player_registry_node));
    pr_head->head->curPlayer = NULL;
    pr_head->head->nextPlayer = NULL;
    pr_head->playerCount = 0;
    if(sem_init(&(pr_head->registryLock), 0, 1) < 0){
        return NULL;
    }
    debug("%li: Initialize player registry", pthread_self());
    return pr_head;
}

void preg_fini(PLAYER_REGISTRY *preg){
    struct player_registry_node* tempNode;
    P(&(preg->registryLock));
    while(preg->head != NULL){ // Free all nodes
        tempNode = preg->head;
        preg->head = preg->head->nextPlayer;
        free(tempNode->curPlayer);
        free(tempNode->nextPlayer);
        free(tempNode);
    }
    V(&(preg->registryLock));
    free(preg); // Free registry itself
    return;
}

PLAYER *preg_register(PLAYER_REGISTRY *preg, char *name){
    PLAYER* newPlayer = player_create(name);
    struct player_registry_node* newPlayerNode = (struct player_registry_node*) Malloc(sizeof(struct player_registry_node));
    newPlayerNode->curPlayer = newPlayer;
    newPlayerNode->nextPlayer = NULL;
    P(&(preg->registryLock));
    struct player_registry_node* tempNode = preg->head;
    if(tempNode->curPlayer == NULL){ // Empty list
        tempNode = newPlayerNode;
        pr_head->playerCount += 1;
        V(&(preg->registryLock));
        return newPlayer;
    }
    while(tempNode->nextPlayer != NULL){
        tempNode = tempNode->nextPlayer;
    }
    tempNode->nextPlayer = newPlayerNode;
    pr_head->playerCount += 1;
    V(&(preg->registryLock));
    return newPlayer;
}
