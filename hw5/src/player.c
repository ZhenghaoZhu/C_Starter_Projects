#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>

#include "client_registry.h"
#include "player_registry.h"
#include "csapp.h"
#include "player.h"
#include "jeux_globals.h"
#include "debug.h"
#include "jeux_helper.h"

typedef struct player {
    char playerName[NAME_MAX_LENGTH];
    int refCount;
    int rating;
    sem_t playerLock;
} PLAYER;

PLAYER *player_create(char *name){
    PLAYER* newPlayer = (PLAYER*) Calloc(1, sizeof(PLAYER));
    if(sem_init(&(newPlayer->playerLock), 0, 1) != 0){
        free(newPlayer);
        return NULL;
    }
    strcat(newPlayer->playerName, name);
    newPlayer->refCount = 1;
    newPlayer->rating = PLAYER_INITIAL_RATING;
    // debug("New Player Created %p, Name %s", newPlayer, newPlayer->playerName);
    return newPlayer;
}

PLAYER *player_ref(PLAYER *player, char *why){
    if(player == NULL){
        return NULL;
    }
    P(&(player->playerLock));
    player->refCount += 1;
    debug("%li: %s (%i -> %i)", pthread_self(), why, player->refCount - 1, player->refCount);
    player->refCount += 1;
    debug("%li: %s (%i -> %i)", pthread_self(), why, player->refCount - 1, player->refCount);
    V(&(player->playerLock));
    return player;
}

void player_unref(PLAYER *player, char *why){
    if(player == NULL){
        return;
    }
    bool isZero = false;
    P(&(player->playerLock));
    player->refCount -= 1;
    debug("%li: %s (%i -> %i)", pthread_self(), why, player->refCount + 1, player->refCount);
    isZero = (player->refCount == 0);
    V(&(player->playerLock));
    if(isZero){
        debug("Player %p reference count reached zero, freeing player", player);
        free(player);
    }
    return;
}

char *player_get_name(PLAYER *player){
    if(player == NULL){
        return NULL;
    }
    return player->playerName;
}

int player_get_rating(PLAYER *player){
    if(player == NULL){
        return 0;
    }
    return player->rating;
}

void player_post_result(PLAYER *player1, PLAYER *player2, int result){
    debug("player_post_result with result %i", result);
    double S1, S2;
    double E1, E2;
    int R1, R2;
    int newR1, newR2;
    if(result == 0){
        S1 = 0.5;
        S2 = 0.5;
    }
    else if(result == 1){
        S1 = 1;
        S2 = 0;
    }
    else{
        S1 = 0;
        S2 = 1;
    }

    P(&(player1->playerLock));
    R1 = player1->rating;
    V(&(player1->playerLock));

    P(&(player2->playerLock));
    R2 = player2->rating;
    V(&(player2->playerLock));

    E1 = 1 + pow(10, ((R2-R1)/400));
    E1 = 1/E1;
    E2 = 1 + pow(10, ((R1-R2)/400));
    E2 = 1/E2;
    newR1 = R1 + (32*(S1 - E1));
    newR2 = R2 + (32*(S2 - E2));

    P(&(player1->playerLock));
    player1->rating = newR1;
    V(&(player1->playerLock));

    P(&(player2->playerLock));
    player2->rating = newR2;
    V(&(player2->playerLock));

    return;
}

