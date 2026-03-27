#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h> 
#include <unistd.h>
#include <stdint.h> 

#include "protocol.h"

int send_tout_message(int descripteur, void *buf, size_t len) {
    size_t envoye = 0; 
    while (envoye < len) {
        size_t req = send(descripteur, (char*)buf + envoye, len - envoye, 0);
    
        if (req <= 0) { 
            return -1; 
        }
        envoye += req; 
    }
    return 0; 
}

int recv_tout_message(int sock, void *buf, size_t len) {
    size_t received = 0;
    while (received < len) {
        ssize_t n = recv(sock, (char *)buf + received, len - received, 0);
        if (n <= 0) return -1;
        received += n;
    }
    return 0;
}

int send_message(int descripteur, MsgType type, void *payload, uint32_t size) {
    MsgHeader header;
    header.type   = type;
    header.length = size;

    // Envoyer le header
    if (send_tout_message(descripteur, &header, sizeof(MsgHeader)) < 0) {
        return -1;
    }

    // Envoyer le payload si nécessaire
    if (payload != NULL && size > 0) {
        if (send_tout_message(descripteur, payload, size) < 0) {
            return -1;
        }
    }

    return 0;
}

int recv_message(int descripteur, MsgHeader *header, void *buf, uint32_t buf_size) {
    // Lire le header d'abord
    if (recv_tout_message(descripteur, header, sizeof(MsgHeader)) < 0) {
        return -1;
    }

    // Sécurité -> vérifier si le buffer destination est assez grand
    if (header->length > buf_size) {
        return -1;
    }

    // Lire le payload si présent
    if (header->length > 0) {
        if (recv_tout_message(descripteur, buf, header->length) < 0) {
            return -1;
        }
    }

    return header->type;
}