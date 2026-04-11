//
// Created by sohaib on 08/04/2026.
//
// Rôle : permettre aux deux joueurs du même room du discuter entre eux

#ifndef CHESS_CHAT_H
#define CHESS_CHAT_H
#include <stdio.h>
#include <time.h> // utiliser pour associer à chaque message l'heurs au quel il est envoyer
// la structure time
typedef struct{
    short heure; // utiliser short pour mois consommer la memoire.
    short minute;
    short seconde;
} msgtime;
// la structure message
typedef struct{
    char username[20];
    char msg[200];
    time momentenv;
}message;
// status du message
typedef enum{
    msg_created,
    msg_send,
    msg_delivered,
    msg_received,
    msg_error,
}messagestat;

#endif //CHESS_CHAT_H