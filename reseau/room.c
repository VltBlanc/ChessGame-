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
    room.nb_spectateurs = 0;
    room.etat = 0;
    rooms[nb_rooms] = room;
    nb_rooms++;
    return 0;
}

int room_rejoindre(Room room,int socket){
    if(room.joueurs[0] == -1){
        room.joueurs[0] = socket;
        return 0;
    }
    else if(room.joueurs[1] == -1){
        room.joueurs[1] = socket;
        return 0;
    }
    else if(room.nb_spectateurs < 10){
        room.spectateurs[room.nb_spectateurs] = socket;
        room.nb_spectateurs++;
        return 0;
    }
    return -1;
}

int room_prete(Room room){
    if(room.joueurs[0] != -1 && room.joueurs[1] != -1){
        room.etat = 1;
        return 0;
    }
    return -1;
}

/*
//Envoie un message à tout le monde dans la room
int room_message(Room room,char* message){
    return -1;
}*/

int room_demarre(Room room){
    if(room.etat == 1){
        room.etat = 2;
        //lancer le jeu?
        return 0;
    }
    return -1;
}