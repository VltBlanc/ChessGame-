#include <stdio.h>       
#include <stdlib.h>      
#include <string.h>      
#include <unistd.h>      
#include <sys/types.h>   
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <pthread.h>
#include "room.h"

#define MAX_ROOMS 100
Room rooms[MAX_ROOMS];
int nb_rooms = 0;

int room_creer(){
    Room room;
    room.joueurs[0] = -1;
    room.joueurs[1] = -1;
    room.spectateurs[0] = -1;
    room.nb_spectateurs = 0; //max 10
    room.etat = 0;
    rooms[nb_rooms] = room;
    nb_rooms++;
    printf("nouvelle room créée : %d\n", nb_rooms);
    return 0;
}

//surement a modif
int room_rejoindre_spectateur(Room room,int socket){
    if(room.nb_spectateurs < 10){
        room.spectateurs[room.nb_spectateurs] = socket;
        room.nb_spectateurs++;
        printf("spectateur rejoint la room %d\n", nb_rooms);
        return 0;
    }
    return -1;
}

int room_prete(Room room){
    if(room.joueurs[0] != -1 && room.joueurs[1] != -1){
        room.etat = 1;
        printf("room %d prete\n", nb_rooms);
        room_demarre(room);
        return 0;
    }
    return -1;
}

/*
//Envoie un message à tout le monde dans la room
int room_message(Room room,char* message){
    return -1;
}*/

//surement a modif
int room_demarre(Room room){
    if(room.etat == 1){
        room.etat = 2;
        //lancer le jeu pour les 2 joueurs?
        printf("room %d en jeu\n", nb_rooms);
        return 0;
    }
    return -1;
}

int room_rejoindre_file(int socket){

    // Si aucune room n'existe, en créer une
    if (nb_rooms == 0) {
        room_creer();
    }

    if (rooms[nb_rooms-1].joueurs[0] == -1){
        rooms[nb_rooms-1].joueurs[0] = socket;   
        printf("joueur 1 rejoint la room %d\n", nb_rooms);
        room_prete(rooms[nb_rooms-1]);
        return 1;
    }
    else if (rooms[nb_rooms-1].joueurs[1] == -1){
        rooms[nb_rooms-1].joueurs[1] = socket;
        printf("joueur 2 rejoint la room %d\n", nb_rooms);
        room_prete(rooms[nb_rooms-1]);
        return 1;
    }
    else{
        room_creer();
        rooms[nb_rooms-1].joueurs[0] = socket;
        printf("joueur 1 rejoint la room %d\n", nb_rooms);
        room_prete(rooms[nb_rooms-1]);
        return 1;
    }
}