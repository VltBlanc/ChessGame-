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

message * preparer_chat(char username[20],char text[200], time_t * derniermsgmoment, int room_id);
int envoyer_chat(int socket ,message * msg_a_envoyer); // renvoie un 1 si le message est envoyer et 0 si l'operation est échoué
message * recevoir_chat(int socket , int room_id); // renvoie le msg reçue
void diffuser_msg_room(Room * distination , message * msg, int socket_emetteur); // enovyer le message à tous les participents du room
void afficher_message(message *msg); // fonction qui affiche le message
#endif //CHESS_CHAT_H