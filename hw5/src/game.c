#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>

#include "client_registry.h"
#include "player_registry.h"
#include "client.h"
#include "jeux_globals.h"
#include "csapp.h"
#include "game.h"
#include "debug.h"
#include "jeux_helper.h"
#include "debug.h"

typedef struct game{
    GAME_ROLE gameWinner;
    GAME_ROLE curRole;
    int refCount;
    bool gameDraw;
    bool terminated;
    char gameBoard[GAME_BOARD_LENGTH];
    int movesMade;
    sem_t gameLock;
} GAME;

typedef struct game_move{
    int boardIndex;
    GAME_ROLE curPlayer;
} GAME_MOVE;

GAME *game_create(void){
    debug("Creating game");
    GAME* newGame = (GAME*) Malloc(sizeof(GAME));
    newGame->gameWinner = NULL_ROLE;
    newGame->curRole = FIRST_PLAYER_ROLE;
    newGame->refCount = 2;
    newGame->gameDraw = false;
    newGame->terminated = false;
    for(int i = 0; i < GAME_BOARD_LENGTH; i++){
        newGame->gameBoard[i] = ' ';
    }
    newGame->movesMade = 0;
    if(sem_init(&(newGame->gameLock), 0, 1) < 0){
        free(newGame);
        return NULL;
    }
    return newGame;
}

GAME *game_ref(GAME *game, char *why){
    debug("Game ref");
    P(&(game->gameLock));
    debug("%s", why);
    game->refCount += 1;
    debug("Increased ref count of game %p (%i -> %i)", game, game->refCount - 1, game->refCount);
    V(&(game->gameLock));
    return game;
}

void game_unref(GAME *game, char *why){
    debug("Game unref");
    bool refCountZero = false;
    P(&(game->gameLock));
    debug("%s", why);
    game->refCount -= 1;
    debug("Decreased ref count of game %p (%i -> %i)", game, game->refCount + 1, game->refCount);
    if(game->refCount == 0){
        refCountZero = true;
    }
    V(&(game->gameLock));
    if(refCountZero){
        debug("Freeing game %p", game);
        free(game);
    }
    return;
}

int game_apply_move(GAME *game, GAME_MOVE *move){
    debug("Game apply move");
    P(&(game->gameLock));
    int curIdx = move->boardIndex; // Remember to subtract
    if(curIdx > 9 || curIdx < 1 || (game->gameBoard[curIdx - 1] != ' ')){
        debug("Passed index out of bounds");
        V(&(game->gameLock));
        return -1;
    }
    curIdx -= 1;
    char boardLetter = ' ';
    if(move->curPlayer == FIRST_PLAYER_ROLE){
        debug("Applying X in board");
        boardLetter = 'X';
        game->curRole = SECOND_PLAYER_ROLE;
    }
    else{
        debug("Applying O in board");
        boardLetter = 'O';
        game->curRole = FIRST_PLAYER_ROLE;
    }
    game->gameBoard[curIdx] = boardLetter;
    game->movesMade += 1;
    V(&(game->gameLock));
    return 0;
}

int game_resign(GAME *game, GAME_ROLE role){
    debug("Game resign");
    P(&(game->gameLock));
    if(game->terminated){
        V(&(game->gameLock));
        return -1;
    }
    if(role == FIRST_PLAYER_ROLE){
        game->gameWinner = SECOND_PLAYER_ROLE;
    }
    else {
        game->gameWinner = FIRST_PLAYER_ROLE;
    }
    V(&(game->gameLock));
    game_unref(game, "Ending GAME");
    return 0;
}

char *game_unparse_state(GAME *game){
    debug("Game unparse state");
    char* retString = (char*) Malloc(GAME_BOARD_STATE_LENGTH + 11);
    char curLine[8];
    P(&(game->gameLock));
    sprintf(curLine, "%c%c%c%c%c%s", game->gameBoard[0], '|', game->gameBoard[1], '|', game->gameBoard[2], "\n");
    strcat(retString, curLine);
    sprintf(curLine, "%c%c%c%c%c%s", '-', '-', '-', '-', '-', "\n");
    strcat(retString, curLine);
    sprintf(curLine, "%c%c%c%c%c%s", game->gameBoard[3], '|', game->gameBoard[4], '|', game->gameBoard[5], "\n");
    strcat(retString, curLine);
    sprintf(curLine, "%c%c%c%c%c%s", '-', '-', '-', '-', '-', "\n");
    strcat(retString, curLine);
    sprintf(curLine, "%c%c%c%c%c%s", game->gameBoard[6], '|', game->gameBoard[7], '|', game->gameBoard[8], "\n");
    strcat(retString, curLine);
    if(game->curRole == FIRST_PLAYER_ROLE){
        strcat(retString, "X to move\n");
    }
    else if(game->curRole == SECOND_PLAYER_ROLE){
        strcat(retString, "O to move\n");
    }
    V(&(game->gameLock));
    return retString;
}

int game_is_over(GAME *game){
    debug("Entering game_is_over");
    char winningLetter = ' ';
    P(&(game->gameLock));
    if(game->movesMade == 9){
        game->gameWinner = NULL_ROLE;
        game->terminated = true;
        debug("Game IS over (DRAW)");
        V(&(game->gameLock));
        return 1;
    }
    // ################################
    if(game->gameBoard[0] != ' ' && game->gameBoard[0] == game->gameBoard[1] && game->gameBoard[0] == game->gameBoard[2]){
        // Start of horizontal checks
        winningLetter = game->gameBoard[0];
    }
    else if(game->gameBoard[3] != ' ' && game->gameBoard[3] == game->gameBoard[4] && game->gameBoard[3] == game->gameBoard[5]){
        winningLetter = game->gameBoard[3];
    }
    else if(game->gameBoard[6] != ' ' && game->gameBoard[6] == game->gameBoard[7] && game->gameBoard[6] == game->gameBoard[8]){
        winningLetter = game->gameBoard[6];
    }
    else if(game->gameBoard[0] != ' ' && game->gameBoard[0] == game->gameBoard[3] && game->gameBoard[0] == game->gameBoard[6]){
        // Start of vertical checks
        winningLetter = game->gameBoard[0];
    }
    else if(game->gameBoard[1] != ' ' && game->gameBoard[1] == game->gameBoard[4] && game->gameBoard[1] == game->gameBoard[7]){
        winningLetter = game->gameBoard[1];
    }
    else if(game->gameBoard[2] != ' ' && game->gameBoard[2] == game->gameBoard[5] && game->gameBoard[2] == game->gameBoard[8]){
        winningLetter = game->gameBoard[2];
    }
    else if(game->gameBoard[0] != ' ' && game->gameBoard[0] == game->gameBoard[4] && game->gameBoard[0] == game->gameBoard[8]){
        // Start of diagonal checks
        winningLetter = game->gameBoard[0];
    }
    else if(game->gameBoard[2] != ' ' && game->gameBoard[2] == game->gameBoard[4] && game->gameBoard[2] == game->gameBoard[6]){
        winningLetter = game->gameBoard[2];
    }
    else{
        winningLetter = ' ';
    }
    // ################################
    if(winningLetter == 'X'){
        game->gameWinner = FIRST_PLAYER_ROLE;
        game->terminated = true;
    }
    else if(winningLetter == 'O'){
        game->gameWinner = SECOND_PLAYER_ROLE;
        game->terminated = true;
    }
    else{
        debug("Game NOT over");
        V(&(game->gameLock));
        return 0;
    }
    debug("Game IS over");
    V(&(game->gameLock));
    return 1;
}

GAME_ROLE game_get_winner(GAME *game){
    debug("Game get winner");
    GAME_ROLE tempGameRole;
    P(&(game->gameLock));
    tempGameRole = game->gameWinner;
    V(&(game->gameLock));
    return tempGameRole;
}

GAME_MOVE *game_parse_move(GAME *game, GAME_ROLE role, char *str){
    debug("Game parse move, move passed: %s", str);
    debug("ROLES! game Role: %i, role : %i", game->curRole, role);
    GAME_MOVE* newGameMove = (GAME_MOVE*) Malloc(sizeof(GAME_MOVE));
    P(&(game->gameLock));
    if(role != NULL_ROLE && game->curRole != role){
        free(newGameMove);
        debug("ROLES DONT MATCH! game Role: %i, role : %i", game->curRole, role);
        V(&(game->gameLock));
        return NULL;
    }
    int curIdx = *str - '0';
    if(curIdx > 9 || curIdx < 1){
        free(newGameMove);
        debug("Wrong Index passed : %i", curIdx);
        V(&(game->gameLock));
        return NULL;
    }
    GAME_ROLE curRole = game->curRole;
    if(strlen(str) == 1){
        newGameMove->boardIndex = curIdx;
    }
    else if(strlen(str) == 4){
        char curLetter = *(str + 3);
        if(curLetter == 'X'){
            curRole = FIRST_PLAYER_ROLE;
        }
        else if(curLetter == 'O'){
            curRole = SECOND_PLAYER_ROLE;
        }
    }
    newGameMove->boardIndex = curIdx;
    newGameMove->curPlayer = curRole;
    V(&(game->gameLock));
    return newGameMove;
}

char *game_unparse_move(GAME_MOVE *move){
    debug("Game unparse move");
    int curIdx = move->boardIndex;
    char intContainer[10];
    sprintf(intContainer, "%d", curIdx);
    int curLineLen = strlen(intContainer) + strlen("<-X");
    char* retStr = Malloc(curLineLen);
    if(move->curPlayer == FIRST_PLAYER_ROLE){
        sprintf(retStr, "%s<-X", intContainer);
    }
    else if(move->curPlayer == SECOND_PLAYER_ROLE){
        sprintf(retStr, "%s<-O", intContainer);
    }
    return retStr;
}