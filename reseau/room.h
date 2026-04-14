#ifndef ROOM_H
#define ROOM_H

typedef struct {
    int joueurs[2];      // sockets des 2 joueurs (-1 si vide)
    int spectateurs[10]; // sockets des spectateurs
    int nb_spectateurs;
    int etat;            // 0=ATTENTE, 1=PRETE, 2=EN_JEU, 3=TERMINEE
} Room;

int room_creer();

int room_rejoindre_spectateur(Room room,int socket);

int room_prete(Room room);

int room_message(Room room,char* message);

int room_demarre(Room room);

int room_rejoindre_file(int socket);

#endif