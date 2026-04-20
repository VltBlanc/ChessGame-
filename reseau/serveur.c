#define _POSIX_C_SOURCE 200809L
#include <stdio.h>       
#include <stdlib.h>      
#include <string.h>      
#include <unistd.h>      
#include <sys/types.h>   
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <pthread.h>
#include <signal.h>
#include "serveur.h"
#include "authentification.h"

// Structure pour passer les infos au thread
typedef struct {
    int socket;
    char pseudo[32];
} client_data_t;

static volatile int g_dS = -1;

void handler_signal(int sig) {
    (void)sig;
    const char msg[] = "\n[serveur] Signal recu, fermeture propre...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    if (g_dS != -1) {
        close(g_dS);
        g_dS = -1;
    }
    _exit(0);
}

void* routine_client(void* arg) {
    client_data_t* data = (client_data_t*)arg;
    int dSC = data->socket;
    char mon_pseudo[32];
    memset(mon_pseudo, 0, 32);

    MsgHeader header;
    char buf[512];

    int login = -1;
    while (login != 5) {
        int type = recv_message(dSC, &header, buf, sizeof(buf));
        if (type < 0) {
            close(dSC);
            free(data);
            pthread_exit(0);
        }

        // On récupère le pseudo si l'auth réussit
        login = traiter_requete_debut(dSC, &header, buf);
        if (login == 5) {
            str_RequAuth *req = (str_RequAuth *)buf;
            strncpy(mon_pseudo, req->username, 31);
        }
    }

    while (1) {
        int type = recv_message(dSC, &header, buf, sizeof(buf));
        if (type <= 0) {
            printf("[serveur] Deconnexion joueur : %s\n", mon_pseudo);
            break;
        }

        // On passe le pseudo à la gestion du jeu
        traiter_requete_jeu(dSC, &header, buf, mon_pseudo);
    }

    close(dSC);
    free(data);
    pthread_exit(0);
}

int traiter_requete_debut(int dSC, MsgHeader *header, void *payload) {
    if (header->type == REQ_CONNEXION) {
        str_RequAuth *req = (str_RequAuth *)payload;
        int ok = logique_create_account(req->username, req->password);

        str_RepAuth rep;
        memset(&rep, 0, sizeof(rep));
        rep.resultat = ok ? STATUS_OK : STATUS_ERR_AUTH;
        strncpy(rep.username, req->username, sizeof(rep.username) - 1);

        send_message(dSC, REP_CONNEXION, &rep, sizeof(rep));
        return ok ? STATUS_OK : STATUS_ERR_AUTH;
    }
    else if (header->type == REQ_AUTH) {
        str_RequAuth *req = (str_RequAuth *)payload;
        int ok = logique_connexion(req->username, req->password);

        str_RepAuth rep;
        memset(&rep, 0, sizeof(rep));
        rep.resultat = ok ? STATUS_OK : STATUS_ERR_AUTH;
        strncpy(rep.username, req->username, sizeof(rep.username) - 1);

        send_message(dSC, REP_AUTH, &rep, sizeof(rep));
        return ok ? 5 : STATUS_ERR_AUTH;
    }
    return -1;
}

// MODIFICATION : Ajout de l'argument pseudo
int traiter_requete_jeu(int dSC, MsgHeader *header, void *payload, const char* pseudo) {
    (void)payload;
    
    if (header->type == REQ_SEARCH) {
        printf("[serveur] %s cherche une partie...\n", pseudo);

        // MODIFICATION : On passe le pseudo pour l'historique
        int room_idx = room_rejoindre_file(dSC, pseudo);

        Room *room = &rooms[room_idx];
        int couleur = (room->joueurs[0] == dSC) ? 1 : 0; 

        str_RepSearch rep;
        memset(&rep, 0, sizeof(rep));
        rep.resultat = STATUS_OK;
        rep.couleur  = couleur;
        // Optionnel: copier l'adversaire ici si votre protocole le permet

        send_message(dSC, REP_SEARCH, &rep, sizeof(rep));

        printf("[serveur] Match trouvé ! %s joue les %s (Room %d)\n",
               pseudo, couleur ? "Blancs" : "Noirs", room_idx);

        room_boucle_jeu(room, dSC, couleur);
        return 0;
    }
    return -1;
}

int lancement_serveur(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = handler_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    int dS = socket(PF_INET, SOCK_STREAM, 0);
    g_dS = dS;

    int opt = 1;
    setsockopt(dS, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(atoi(argv[1]));

    if(bind(dS, (struct sockaddr*)&ad, sizeof(ad)) < 0){
        perror("Erreur bind");
        exit(1);
    }

    listen(dS, 7);

    while (1){
        struct sockaddr_in aC;
        socklen_t lg = sizeof(struct sockaddr_in);
        int dSC = accept(dS, (struct sockaddr*) &aC, &lg);
        if (dSC < 0) continue;
        
        client_data_t* data = malloc(sizeof(client_data_t));
        data->socket = dSC;

        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, routine_client, data) != 0){
            free(data);
            close(dSC);
            continue;
        }
        pthread_detach(thread_id);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    lancement_serveur(argc, argv);
    return 0;
}