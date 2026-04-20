#ifndef ROOM_H
#define ROOM_H

#include <pthread.h>
#include "protocol.h"
#include "historique.h"

typedef struct {
    int joueurs[2];      // sockets des 2 joueurs (-1 si vide)
    int spectateurs[10]; // sockets des spectateurs
    int nb_spectateurs;
    int etat;            // 0=ATTENTE, 1=PRETE, 2=EN_JEU, 3=TERMINEE

    // Synchronisation : protège cette room + signal quand les 2 joueurs sont là
    pthread_mutex_t mutex;
    pthread_cond_t  cond_prete;

    // État du jeu
    int trait;           // 0 = joueur[0] joue (blanc), 1 = joueur[1] joue (noir)

    // Historique de la partie
    historique_partie hist;
    char username_blanc[32];
    char username_noir[32];
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
int room_rejoindre_file(int socket, const char *username);

// Boucle de jeu : relaye les coups entre les 2 joueurs
void room_boucle_jeu(Room *room, int mon_socket, int ma_couleur);

#endif // ROOM_H