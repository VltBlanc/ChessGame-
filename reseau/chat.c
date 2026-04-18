#include <stdio.h>
#include <time.h>
#include "chat.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

// Anti - spam
#define DELAI_MESSAGE 5

// il s'agit de la fonction qui prepare les messgaes à envoyer
message * preparer_chat(char username[20] ,char text[200],time_t * derniermsgmoment,int room_id){
    time_t now = time(NULL);
    // verification contre le spam
    if (difftime(now, *derniermsgmoment) < DELAI_MESSAGE) {
        return NULL;
    }
    message *tosend = malloc(sizeof(message));
    if (tosend == NULL){
        return NULL;
    }

    struct tm *tm = localtime(&now);
    strncpy(tosend->msg, text,199);
    strncpy(tosend->username, username,19);
    tosend->momentenv.heure = (short)tm->tm_hour;
    tosend->momentenv.minute = (short)tm->tm_min;
    tosend->room_id = room_id;
    *derniermsgmoment = now;
    return tosend;
}

// la fonction qui fait envoyer les messages au serveur

int envoyer_chat(int socket ,message * msg_a_envoyer){
    if (msg_a_envoyer == NULL) return -1;
    // Envoi de la structure complète à rediriger par le serveur selon le room_id
    ssize_t bytes_sent = send(socket, msg_a_envoyer, sizeof(message), 0);
    // verification
    if (bytes_sent < 0) {
        perror("Erreur lors de l'envoi du message");
        return -1;
    }
    // Libération de la mémoire allouée par preparer_chat
    free(msg_a_envoyer);
    return 0;
}

message * recevoir_chat(int socket , int room_id){
    message * msg_reçu = malloc(sizeof(message));
    if (msg_reçu== NULL){
        return NULL;
    }
    // choix du flag : MSG_WAITALL est pour : - le thread qui gere la reception des msg bloque jusqu'a que sizeof(message) est reçu.
    //                                        - Garantit l'intégrité de la structure et évite de traiter un message tronqué par la segmentation TCP.
    ssize_t bytes_reçu = recv(socket, msg_reçu, sizeof(message), MSG_WAITALL);
    if (bytes_reçu < 0){ // cas d'erreur
        free(msg_reçu);
        return NULL;
    }
    return msg_reçu;
}
// !!! petit problème pour le moment : le joueur reçoie le msg de lui même !!! ( une premier idée c'est de le gerer chez le joueur ( igorer le msg de lui même )
void diffuser_msg_room(Room * distination , message * msg,int socket_emetteur){
    if (distination == NULL || msg == NULL){return;} // gestion d'erreur
    // envoyer au joueur
    for ( int i = 0 ; i<2 ; i ++){
        if (distination->joueurs[i] != -1 && distination->joueurs[i] != socket_emetteur) {
            // MSG_DONTWAIT pour ne pas bloquer le serveur si un joueur lag
            send(distination->joueurs[i], msg, sizeof(message), MSG_DONTWAIT);
        }
    }
    // envoyer au spectateurs
    for (int i = 0; i < distination->nb_spectateurs; i++) {
        if (distination->spectateurs[i] != -1 && distination->spectateurs[i] != socket_emetteur) {
            send(distination->spectateurs[i], msg, sizeof(message), MSG_DONTWAIT);
        }
    }
}