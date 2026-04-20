#ifndef ROOM_H
#define ROOM_H

#include <pthread.h>
#include "protocol.h"
#include "../reseau/historique.h"

typedef struct {
    int joueurs[2];      
    int spectateurs[10]; 
    int nb_spectateurs;
    int etat;            
    
    // Nouveaux champs pour l'historique
    char pseudo_blanc[33];
    char pseudo_noir[33];
    historique_partie historique;

    pthread_mutex_t mutex;
    pthread_cond_t  cond_prete;
    int trait;           
} Room;

// Tableau global de rooms (défini dans room.c)
extern Room rooms[];
extern int nb_rooms;
extern pthread_mutex_t mutex_rooms;

int room_creer();

int room_rejoindre_spectateur(Room *room, int socket);
int room_demarre(Room *room);

// Matchmaking : rejoindre la file d'attente (bloquant pour le J1)
// Retourne l'index de la room
int room_rejoindre_file(int socket, const char *pseudo);

// Boucle de jeu : relaye les coups entre les 2 joueurs
void room_boucle_jeu(Room *room, int mon_socket, int ma_couleur);

#endif // ROOM_H