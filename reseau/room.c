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
#include "historique.h"

#define MAX_ROOMS 100
Room rooms[MAX_ROOMS];
int nb_rooms = 0;
pthread_mutex_t mutex_rooms = PTHREAD_MUTEX_INITIALIZER;

int room_creer(){
    Room room;
    room.joueurs[0] = -1;
    room.joueurs[1] = -1;
    room.spectateurs[0] = -1;
    room.nb_spectateurs = 0; 
    room.etat = 0;
    room.trait = 0; 

    pthread_mutex_init(&room.mutex, NULL);
    pthread_cond_init(&room.cond_prete, NULL);

    rooms[nb_rooms] = room;
    nb_rooms++;
    printf("Nouvelle room créée : %d\n", nb_rooms);
    return 0;
}

int room_rejoindre_spectateur(Room *room, int socket){
    if(room->nb_spectateurs < 10){
        room->spectateurs[room->nb_spectateurs] = socket;
        room->nb_spectateurs++;
        return 0;
    }
    return -1;
}

int room_demarre(Room *room){
    if(room->etat == 1){
        room->etat = 2;
        room->trait = 0; 
        printf("Room en jeu\n");
        return 0;
    }
    return -1;
}

// CORRECTION : Signature correspondant à votre room.h (avec pseudo)
int room_rejoindre_file(int socket, const char *pseudo){
    pthread_mutex_lock(&mutex_rooms);

    if (nb_rooms == 0 || rooms[nb_rooms-1].joueurs[1] != -1) {
        room_creer();
    }
    Room *room = &rooms[nb_rooms-1];

    if (room->joueurs[0] == -1) {
        // Premier joueur
        room->joueurs[0] = socket;
        strncpy(room->pseudo_blanc, pseudo, 32); // On stocke son pseudo
        printf("Joueur 1 (%s) rejoint la room %d\n", pseudo, nb_rooms);
        pthread_mutex_unlock(&mutex_rooms);

        pthread_mutex_lock(&room->mutex);
        while (room->etat < 1) {
            pthread_cond_wait(&room->cond_prete, &room->mutex);
        }
        pthread_mutex_unlock(&room->mutex);

        return nb_rooms - 1;
    } else {
        // Deuxième joueur
        room->joueurs[1] = socket;
        strncpy(room->pseudo_noir, pseudo, 32); // On stocke son pseudo
        printf("Joueur 2 (%s) rejoint la room %d\n", pseudo, nb_rooms);

        // INITIALISATION HISTORIQUE avec les pseudos enregistrés
        historique_init(&room->historique, room->pseudo_blanc, room->pseudo_noir);

        pthread_mutex_lock(&room->mutex);
        room->etat = 1;
        room_demarre(room);  
        pthread_cond_signal(&room->cond_prete);  
        pthread_mutex_unlock(&room->mutex);

        pthread_mutex_unlock(&mutex_rooms);
        return nb_rooms - 1;
    }
}

void room_boucle_jeu(Room *room, int mon_socket, int ma_couleur) {
    (void)ma_couleur;
    MsgHeader header;
    char buf[512];

    while (room->etat == 2) {
        int type = recv_message(mon_socket, &header, buf, sizeof(buf));
        
        // GESTION FIN DE PARTIE / ABANDON / DECONNEXION
        if (type <= 0 || header.type == REQ_RESIGN) {
            printf("[room] Déconnexion ou abandon détecté\n");
            
            if(room->etat == 2) {
                int mon_index = (room->joueurs[0] == mon_socket) ? 0 : 1;
                int adversaire = room->joueurs[mon_index == 0 ? 1 : 0];
                
                // Déterminer les raisons pour l'historique et le réseau
                int res_hist = (mon_index == 0) ? RESULTAT_PARTIE_NOIR_WIN : RESULTAT_PARTIE_WHITE_WIN;
                int raison_hist = (type <= 0) ? RAISON_DISCONNECT : RAISON_RESIGN;
                int raison_net = (type <= 0) ? GAME_DISCONNECT : GAME_RESIGN;

                // SAUVEGARDE FINALE
                historique_finaliser(&room->historique, res_hist, raison_hist);

                if (adversaire != -1) {
                    str_RepResultatPartie rep;
                    memset(&rep, 0, sizeof(rep));
                    rep.raison = raison_net;
                    rep.gagnant = (mon_index == 0) ? 0 : 1; 
                    send_message(adversaire, REP_RESULTAT_PARTIE, &rep, sizeof(rep));
                }
            }
            room->etat = 3; 
            break;
        }

        if (header.type == REQ_MOVE) {
            str_ReqMove *req = (str_ReqMove *)buf;

            int mon_index = (room->joueurs[0] == mon_socket) ? 0 : 1;
            
            // Vérification du tour
            if (room->trait != mon_index) {
                str_RepMove rep_move;
                memset(&rep_move, 0, sizeof(rep_move));
                rep_move.resultat = STATUS_ERR_MOVE;
                send_message(mon_socket, REP_MOVE_ACK, &rep_move, sizeof(rep_move));
                continue;
            }

            // ENREGISTREMENT DU COUP DANS L'HISTORIQUE
            move_t coup_h;
            memset(&coup_h, 0, sizeof(move_t));
            coup_h.start = (square_t){.rank = req->dep_ligne, .file = req->dep_col};
            coup_h.end   = (square_t){.rank = req->vers_ligne, .file = req->vers_col};
            coup_h.promotion = ' '; 
            
            historique_enregistrer_coup(coup_h, &room->historique);

            // ACK au joueur actuel
            str_RepMove rep_ack;
            memset(&rep_ack, 0, sizeof(rep_ack));
            rep_ack.resultat = STATUS_OK;
            send_message(mon_socket, REP_MOVE_ACK, &rep_ack, sizeof(rep_ack));

            // Relais à l'adversaire
            int adversaire = room->joueurs[mon_index == 0 ? 1 : 0];
            str_RepMove rep_move;
            memset(&rep_move, 0, sizeof(rep_move));
            rep_move.resultat = STATUS_OK;
            rep_move.trait = (room->trait == 0) ? 1 : 0; 
            rep_move.dep_col = req->dep_col;
            rep_move.dep_ligne = req->dep_ligne;
            rep_move.vers_col = req->vers_col;
            rep_move.vers_ligne = req->vers_ligne;

            send_message(adversaire, REP_MOVE, &rep_move, sizeof(rep_move));
            
            // Changement de trait
            room->trait = (room->trait == 0) ? 1 : 0;
        }
    }
}