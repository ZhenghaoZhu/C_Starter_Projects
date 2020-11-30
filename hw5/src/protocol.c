#include "protocol.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "csapp.h"

/* 
 * The htonl() function converts the unsigned integer hostlong (uint32_t) from host byte order to network byte order.
 * The htons() function converts the unsigned short integer hostshort (uint16_t) from host byte order to network byte order.
 * The ntohl() function converts the unsigned integer netlong (uint32_t) from network byte order to host byte order.
 * The ntohs() function converts the unsigned short integer netshort (uint16_t) from network byte order to host byte order. 
 */

int proto_send_packet(int fd, JEUX_PACKET_HEADER *hdr, void *data){
    // hdr in network-byte order 
    uint16_t payloadSize = htons(hdr->size);

    if(rio_writen(fd, hdr, sizeof(JEUX_PACKET_HEADER)) == -1){
        return -1;
    }

    if(payloadSize > 0){
        if(rio_writen(fd, data, payloadSize) == -1){
            return -1;
        }
        else{
            return 0;
        }
    }

    return 0;

}

int proto_recv_packet(int fd, JEUX_PACKET_HEADER *hdr, void **payloadp){
    if(rio_readn(fd, hdr, sizeof(JEUX_PACKET_HEADER)) == -1){
        return -1;
    }

    hdr->size = ntohs(hdr->size);
    hdr->timestamp_sec = ntohl(hdr->timestamp_sec);
    hdr->timestamp_nsec = ntohl(hdr->timestamp_nsec);

    if(hdr->size > 0){
        void* tempPayloadp = (void*) Malloc(hdr->size + 1);
        ((char*)tempPayloadp)[hdr->size] = '\0';
        if(rio_readn(fd, tempPayloadp, hdr->size) == -1){
            return -1;
        }
        *payloadp = tempPayloadp;
    }

    return 0;
}