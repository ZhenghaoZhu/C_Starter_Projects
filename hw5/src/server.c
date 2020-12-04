#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

#include "client_registry.h"
#include "csapp.h"
#include "jeux_helper.h"
#include "jeux_globals.h"
#include "debug.h"
#include "client.h"
#include "server.h"
#include "protocol.h"

void *jeux_client_service(void *arg){
    int* intArg = (int*) arg;
    int fd = *intArg;
    pthread_detach(pthread_self());
    CLIENT* curClient = creg_register(client_registry, fd);
    free(arg); // Free passed arg
    bool loggedIn = false;
    debug("Client %p loggedIn Status: %i", curClient, loggedIn);
    while(true){
        JEUX_PACKET_HEADER hdr = {JEUX_NO_PKT};
        void* payload = NULL;
        debug("Trying to receive packet with proto_recv_packet");
        if(proto_recv_packet(fd, &hdr, &payload) == -1){ // Get header and payload
            break; // Get out of loop
        }
        switch (hdr.type){
            case JEUX_LOGIN_PKT:
                if(!loggedIn){
                    debug("Received JEUX_LOGIN_PKT");
                    PLAYER* curPlayer = preg_register(player_registry, payload);
                    if((client_login(curClient, curPlayer)) == 0){
                        debug("Client %p logged in successfully", curClient);
                        loggedIn = true;
                        client_send_ack(curClient, NULL, 0); // Empty payload
                    }
                    else{
                        debug("Client %p logged in UNSUCcESSFULLY", curClient);
                        client_send_nack(curClient);
                    }
                }
                else{
                    fprintf(stderr, "ERROR");
                    fflush(stderr);
                    client_send_nack(curClient);
                }
                break;

            case JEUX_USERS_PKT:
                if(loggedIn){
                    debug("Received JEUX_USERS_PKT");
                    PLAYER** playerArr = creg_all_players(client_registry);
                    PLAYER* tempPlayer = *(playerArr + 0);
                    char* retStr = NULL;
                    int curLineLen = 0;
                    int i = 1; // Already got first element of array
                    while(tempPlayer != NULL){
                        char ratingContainer[10];
                        sprintf(ratingContainer, "%d", player_get_rating(tempPlayer));
                        curLineLen = strlen(player_get_name(tempPlayer)) + strlen("\t") + strlen(ratingContainer) + strlen("\n");
                        retStr = realloc(retStr, curLineLen);
                        strcat(retStr, player_get_name(tempPlayer));
                        strcat(retStr, "\t");
                        strcat(retStr, ratingContainer);
                        strcat(retStr, "\n");
                        tempPlayer = *(playerArr + i);
                        i += 1;
                    }
                    struct timespec retTime;
                    clock_gettime(CLOCK_MONOTONIC, &retTime);
                    JEUX_PACKET_HEADER retHdr = {JEUX_ACK_PKT, 0, 0, htons(strlen(retStr)), htonl(retTime.tv_sec), htonl(retTime.tv_nsec)};
                    client_send_packet(curClient, &retHdr, retStr);
                }
                else{
                    fprintf(stderr, "ERROR");
                    fflush(stderr);
                    client_send_nack(curClient);
                }
                break;
            
            case JEUX_INVITE_PKT:
                if(loggedIn){
                    debug("Received JEUX_INVITE_PKT");
                    CLIENT* otherClient = creg_lookup(client_registry, payload);
                    int senderRole = hdr.role;
                    int targetRole = 2;
                    if(senderRole == 2){
                        targetRole = 1;
                    }
                    int invID = client_make_invitation(curClient, otherClient, targetRole, senderRole);
                    if(invID != -1){
                        struct timespec retTime;
                        clock_gettime(CLOCK_MONOTONIC, &retTime);
                        JEUX_PACKET_HEADER retHdr = {JEUX_ACK_PKT, invID, 0, 0, htonl(retTime.tv_sec), htonl(retTime.tv_nsec)};
                        client_send_packet(curClient, &retHdr, NULL);
                    }
                    else{
                        client_send_nack(curClient);
                    }
                }
                else{
                    fprintf(stderr, "ERROR");
                    fflush(stderr);
                    client_send_nack(curClient);
                }
                break;

            case JEUX_REVOKE_PKT:
                if(loggedIn){
                    debug("Received JEUX_REVOKE_PKT");
                    if((client_revoke_invitation(curClient, hdr.id)) == 0){
                        client_send_ack(curClient, NULL, 0);
                    }
                    else{
                        client_send_nack(curClient);
                    }
                }
                else{
                    fprintf(stderr, "ERROR");
                    fflush(stderr);
                    client_send_nack(curClient);
                }
                break;

            case JEUX_DECLINE_PKT:
                if(loggedIn){
                    debug("Received JEUX_DECLINE_PKT");
                    if((client_decline_invitation(curClient, hdr.id)) == 0){
                        client_send_ack(curClient, NULL, 0);
                    }
                    else{
                        client_send_nack(curClient);
                    }
                }
                else{
                    fprintf(stderr, "ERROR");
                    fflush(stderr);
                    client_send_nack(curClient);
                }
                break;

            case JEUX_ACCEPT_PKT:
                if(loggedIn){
                    debug("Received JEUX_ACCEPT_PKT");
                    char* retMsg = NULL;
                    if((client_accept_invitation(curClient, hdr.id, &   retMsg)) == 0){
                        if(retMsg != NULL){
                            struct timespec retTime;
                            clock_gettime(CLOCK_MONOTONIC, &retTime);
                            JEUX_PACKET_HEADER retHdr = {JEUX_ACK_PKT, 0, 0, htons(strlen(retMsg)), htonl(retTime.tv_sec), htonl(retTime.tv_nsec)};
                            client_send_packet(curClient, &retHdr, retMsg);
                            free(retMsg);
                        }
                        else{
                            client_send_ack(curClient, NULL, 0);
                        }
                    }
                    else{
                        client_send_nack(curClient);
                    }
                }
                else{
                    fprintf(stderr, "ERROR");
                    fflush(stderr);
                    client_send_nack(curClient);
                }
                break;

            case JEUX_MOVE_PKT:
                if(loggedIn){
                    debug("Received JEUX_MOVE_PKT");
                    if((client_make_move(curClient, hdr.id, payload)) == 0){ // TODO 
                        client_send_ack(curClient, NULL, 0);
                    }
                    else{
                        client_send_nack(curClient);
                    }
                }
                else{
                    fprintf(stderr, "ERROR");
                    fflush(stderr);
                    client_send_nack(curClient);
                }
                break;

            case JEUX_RESIGN_PKT:
                if(loggedIn){
                    debug("Received JEUX_RESIGN_PKT");
                    if((client_resign_game(curClient, hdr.id)) == 0){
                        client_send_ack(curClient, NULL, 0);
                    }
                    else{
                        client_send_nack(curClient);
                    }
                }
                else{
                    fprintf(stderr, "ERROR");
                    fflush(stderr);
                    client_send_nack(curClient);
                }
                break;
            
            default:
                fprintf(stderr, "ERROR");
                fflush(stderr);
                client_send_nack(curClient);
                break;
        }
    }
    return NULL;
}

