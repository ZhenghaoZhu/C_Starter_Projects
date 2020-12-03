#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>

#include "player.h"
#include "semaphore.h"
#include "csapp.h"
#include "jeux_helper.h"
#include "debug.h"

typedef struct player {
    char playerName[NAME_MAX_LENGTH];
    int refCount;
    int rating;
    pthread_mutex_t playerLock;
} PLAYER;

PLAYER *player_create(char *name){
    PLAYER* newPlayer = (PLAYER*) Calloc(1, sizeof(PLAYER));
    if(pthread_mutex_init(&(newPlayer->playerLock), NULL) != 0){
        free(newPlayer);
        return NULL;
    }
    strncpy(newPlayer->playerName, name, strlen(name));
    newPlayer->refCount = 1;
    newPlayer->rating = PLAYER_INITIAL_RATING;
    // debug("New Player Created %p, Name %s", newPlayer, newPlayer->playerName);
    return newPlayer;
}

PLAYER *player_ref(PLAYER *player, char *why){
    if(player == NULL){
        return NULL;
    }
    pthread_mutex_lock(&(player->playerLock));
    player->refCount += 1;
    debug("%li: Increase reference count on player %p (%i -> %i)", pthread_self(), player, player->refCount - 1, player->refCount);
    player->refCount += 1;
    debug("%li: Increase reference count on player %p (%i -> %i)", pthread_self(), player, player->refCount - 1, player->refCount);
    pthread_mutex_unlock(&(player->playerLock));
    return player;
}

void player_unref(PLAYER *player, char *why){
    if(player == NULL){
        return;
    }
    bool isZero = false;
    pthread_mutex_lock(&(player->playerLock));
    player->refCount -= 1;
    debug("%li: Decrease reference count on player %p (%i -> %i)", pthread_self(), player, player->refCount + 1, player->refCount);
    isZero = (player->refCount == 0);
    pthread_mutex_unlock(&(player->playerLock));
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

    pthread_mutex_lock(&(player1->playerLock));
    R1 = player1->rating;
    pthread_mutex_unlock(&(player1->playerLock));

    pthread_mutex_lock(&(player2->playerLock));
    R2 = player2->rating;
    pthread_mutex_unlock(&(player2->playerLock));

    E1 = 1 + pow(10, ((R2-R1)/400));
    E1 = 1/E1;
    E2 = 1 + pow(10, ((R1-R2)/400));
    E2 = 1/E2;
    newR1 = R1 + (32*(S1 - E1));
    newR2 = R2 + (32*(S2 - E2));

    pthread_mutex_lock(&(player1->playerLock));
    player1->rating = newR1;
    pthread_mutex_unlock(&(player1->playerLock));

    pthread_mutex_lock(&(player2->playerLock));
    player2->rating = newR2;
    pthread_mutex_unlock(&(player2->playerLock));

    return;
}

