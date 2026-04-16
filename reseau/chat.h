// Rôle : permettre aux deux joueurs du même room du discuter entre eux

#ifndef CHESS_CHAT_H
#define CHESS_CHAT_H
#include <stdio.h>
#include <time.h> // utiliser pour associer à chaque message l'heurs au quel il est envoyer
#include "room.h"
// la structure time
typedef struct{
    short heure; // utiliser short pour mois consommer la memoire.
    short minute;
} msgtime;
// la structure message
typedef struct{
    char username[20];
    char msg[200];
    msgtime momentenv;
    int room_id;
}message;
// status du message
typedef enum{
    msg_created,
    msg_send,
    msg_delivered,
    msg_received,
    msg_error,
}messagestat;

message * preparer_chat(char username[20],char text[200], time_t delai_contre_spam);
int envoyer_chat();
int recevoir_chat();
#endif //CHESS_CHAT_H