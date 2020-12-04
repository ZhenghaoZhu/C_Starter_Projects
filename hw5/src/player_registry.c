#include <stddef.h>

#include "player_registry.h"
#include "player.h"
#include "semaphore.h"
#include "csapp.h"
#include "debug.h"
#include "jeux_helper.h"

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
    pr_head->head = NULL;
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
        // free(tempNode->curPlayer);
        // free(tempNode->nextPlayer);
        free(tempNode);
    }
    V(&(preg->registryLock));
    free(preg->head); // Free head itself
    free(preg); // Free registry itself
    return;
}

PLAYER *preg_register(PLAYER_REGISTRY *preg, char *name){
    debug("Register player %s", name);
    PLAYER* newPlayer = player_create(name);
    struct player_registry_node* newPlayerNode = (struct player_registry_node*) Malloc(sizeof(struct player_registry_node));
    newPlayerNode->curPlayer = newPlayer;
    newPlayerNode->nextPlayer = NULL;
    P(&(preg->registryLock));
    if(preg_player_exists(preg, name) != NULL){
        free(newPlayer);
        free(newPlayerNode);
        debug("Player with name %s already exists", name);
        V(&(preg->registryLock));
        return preg_player_exists(preg, name);
    }
    struct player_registry_node* tempNode = preg->head;
    if(preg->head == NULL){ // Empty list
        preg->head = newPlayerNode;
        preg->playerCount += 1;
        V(&(preg->registryLock));
        return newPlayer;
    }
    while(tempNode->nextPlayer != NULL){
        tempNode = tempNode->nextPlayer;
    }
    tempNode->nextPlayer = newPlayerNode;
    preg->playerCount += 1;
    V(&(preg->registryLock));
    return newPlayer;
}

PLAYER* preg_player_exists(PLAYER_REGISTRY *preg, char *name){
    struct player_registry_node* tempNode = preg->head;
    if(preg->head == NULL){
        return NULL;
    }
    if((tempNode->curPlayer != NULL) && (strcmp(player_get_name(tempNode->curPlayer), name) == 0)){
        return tempNode->curPlayer;
    }
    while(tempNode->nextPlayer != NULL){
        if(strcmp(player_get_name(tempNode->curPlayer), name) == 0){
            return tempNode->curPlayer;
        }
        tempNode = tempNode->nextPlayer;
    }
    return NULL;
}
