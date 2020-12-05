#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>

#include "client_registry.h"
#include "player_registry.h"
#include "invitation.h"
#include "client.h"
#include "player.h"
#include "server.h"
#include "csapp.h"
#include "debug.h"
#include "protocol.h"

typedef struct invitation{
    GAME* invGame;
    CLIENT* invSource;
    CLIENT* invTarget;
    GAME_ROLE invSourceGameRole;
    GAME_ROLE invTargetGameRole;
    int invState;
    int refCount;
    sem_t invLock;
} INVITATION;

INVITATION *inv_create(CLIENT *source, CLIENT *target, GAME_ROLE source_role, GAME_ROLE target_role){
    if(source == NULL || target == NULL){
        return NULL;
    }
    if(client_get_fd(source) == client_get_fd(target)){
        return NULL;
    }
    INVITATION* newInv = (INVITATION*) Malloc(sizeof(INVITATION));
    newInv->invGame = NULL;
    client_ref(source, "Creating invitation for source");
    newInv->invSource = source;
    client_ref(target, "Creating invitation for target");
    newInv->invTarget = target;
    newInv->invSourceGameRole = source_role;
    newInv->invTargetGameRole = target_role;
    newInv->invState = INV_OPEN_STATE;
    newInv->refCount = 1;
    if(sem_init(&(newInv->invLock), 0, 1) < 0){
        free(newInv);
        return NULL;
    }
    return newInv;
}

INVITATION *inv_ref(INVITATION *inv, char *why){
    P(&(inv->invLock));
    debug("%s", why);
    inv->refCount += 1;
    V(&(inv->invLock));
    return inv;
}

void inv_unref(INVITATION *inv, char *why){
    bool refCountZero = false;
    P(&(inv->invLock));
    debug("%s", why);
    inv->refCount -= 1;
    if(inv->refCount == 0){
        refCountZero = true;
    }
    V(&(inv->invLock));
    if(refCountZero){
        debug("Freeing invitation %p", inv);
        free(inv);
    }
    return;
}

CLIENT *inv_get_source(INVITATION *inv){
    CLIENT* tempClient = NULL;
    P(&(inv->invLock));
    tempClient = inv->invSource;
    V(&(inv->invLock));
    return tempClient;
}

CLIENT *inv_get_target(INVITATION *inv){
    CLIENT* tempClient = NULL;
    P(&(inv->invLock));
    tempClient = inv->invTarget;
    V(&(inv->invLock));
    return tempClient;
}

GAME_ROLE inv_get_source_role(INVITATION *inv){
    GAME_ROLE tempGameRole;
    P(&(inv->invLock));
    tempGameRole = inv->invSourceGameRole;
    V(&(inv->invLock));
    return tempGameRole;
}

GAME_ROLE inv_get_target_role(INVITATION *inv){
    GAME_ROLE tempGameRole;
    P(&(inv->invLock));
    tempGameRole = inv->invTargetGameRole;
    V(&(inv->invLock));
    return tempGameRole;
}

GAME *inv_get_game(INVITATION *inv){
    GAME* tempGame = NULL;
    P(&(inv->invLock));
    tempGame = inv->invGame;
    V(&(inv->invLock));
    return tempGame;
}

int inv_accept(INVITATION *inv){
    P(&(inv->invLock));
    if(inv->invState == INV_OPEN_STATE){
        inv->invState = INV_ACCEPTED_STATE;
        inv->invGame = game_create();
    }
    else{
        V(&(inv->invLock));
        return -1;
    }
    V(&(inv->invLock));
    return 0;
}

int inv_close(INVITATION *inv, GAME_ROLE role){
    P(&(inv->invLock));
    if(inv->invState == INV_OPEN_STATE || inv->invState == INV_ACCEPTED_STATE){
        if(role == NULL_ROLE){
            inv->invState = INV_CLOSED_STATE;
        }
        else{
            inv->invState = INV_CLOSED_STATE;
            if(game_is_over(inv->invGame) == 0){
                game_resign(inv->invGame, role);
            }
        }
        
    }
    else{
        return -1;
    }
    V(&(inv->invLock));
    return 0;
}