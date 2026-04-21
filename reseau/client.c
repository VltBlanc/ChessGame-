#define _POSIX_C_SOURCE 200809L
#include <stdio.h>       
#include <stdlib.h>      
#include <string.h>      
#include <unistd.h>      
#include <sys/types.h>   
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h> 
#include <signal.h>
#include "protocol.h"
#include "client.h"
#include "../src/game.h"

// Socket client globale pour le nettoyage lors d'un signal
static volatile int g_dS_client = -1;

// Gestionnaire de signal pour fermeture propre (Ctrl+C, kill, etc.)
void handler_signal_client(int sig) {
    printf("\n[client] Signal %d reçu, fermeture propre...\n", sig);

    if (g_dS_client != -1) {
        close(g_dS_client);
        g_dS_client = -1;
    }

    printf("[client] Socket fermée, arrêt.\n");
    exit(0);
}



// pour lancer :
// ./client IP_serveur numero_du_port
// si serveur sur le meme pc IP=127.0.0.1
int lancement_client(int argc, char *argv[]) {

    if (argc < 3) {
        perror("Erreur : Tu dois préciser l'ip et le port du serveur");
        exit(1);
    }

    // Enregistrement des gestionnaires de signaux
    struct sigaction sa;
    sa.sa_handler = handler_signal_client;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);   // Ctrl+C
    sigaction(SIGTERM, &sa, NULL);  // kill

    //creation socket
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS<0){
        perror("Erreur création socket client");
        exit(1);
    }
    g_dS_client = dS; // sauvegarder pour le handler de signal

    //on va utiliser IPV4
    struct sockaddr_in aS;
    aS.sin_family = AF_INET;

    //conversion en binaire du texte et du port
    inet_pton(AF_INET,argv[1],&(aS.sin_addr));
    aS.sin_port = htons(atoi(argv[2]));
    socklen_t lgA = sizeof(struct sockaddr_in);

    //demande connection
    if(connect(dS, (struct sockaddr *) &aS, lgA)<0){
        perror("Erreur pour se connecter au serveur");
        close(dS);
        exit(1);
    }

    //communication
    char m[20];

    //login=1 quand le auth est reussi
    int login = -1;
    while(login != 1){
        memset(m, 0, sizeof(m));
        printf("commande disponible : auth, creer compte\n");

        printf("Entrer une commande : ");
        fgets(m, sizeof(m), stdin);
        m[strcspn(m, "\n")] = '\0'; // retirer le \n

        login = traiter_requete_debut(dS, m); 
    }

    while (1) {
        memset(m, 0, sizeof(m));
        printf("\ncommandes disponibles : chercher partie, cherche partie sep, quit\n");
        printf("Entrer une commande : ");

        if (fgets(m, sizeof(m), stdin) == NULL) break;
        m[strcspn(m, "\n")] = '\0';

        if (strcmp(m, "quit") == 0) break;

        // On récupère la couleur (1, 0) ou le mode spectateur (-1)
        int couleur_ou_mode = traiter_requete_jeu(dS, m);

        // On lance la fenêtre de jeu
        if (couleur_ou_mode >= -1) {
            printf("[client] Lancement de l'interface graphique...\n");
            game_network_play(dS, couleur_ou_mode);
        }
    }

    //fermer socket
    close (dS);

    return 0;
}

int traiter_requete_debut(int dS, char *m){
    if(strcmp(m, "creer compte") == 0){
        str_RequAuth req;
        memset(&req, 0, sizeof(req));

        printf("username : ");
        scanf("%31s", req.username);
        printf("password : ");
        scanf("%31s", req.password);
        int c; while ((c = getchar()) != '\n' && c != EOF); // vider le buffer

        if(send_message(dS, REQ_CONNEXION, &req, sizeof(req)) < 0){
            perror("Erreur pour envoyer message");
            close(dS);
            return -1;
        }
        
        // Recevoir la réponse du serveur
        MsgHeader header;
        str_RepAuth rep;
        memset(&rep, 0, sizeof(rep));

        if(recv_message(dS, &header, &rep, sizeof(rep)) < 0){
            perror("Erreur pour recevoir la réponse");
            close(dS);
            return -1;
        }

        if(rep.resultat == STATUS_OK){
            printf("Compte créé avec succès pour '%s' !\n", rep.username);
            return 0;
        } else {
            printf("Erreur : le pseudo '%s' existe déjà.\n", rep.username);
            return -1;
        }   
    }
    else if(strcmp(m, "auth") == 0){
        str_RequAuth req;
        memset(&req, 0, sizeof(req));

        printf("username : ");
        scanf("%31s", req.username);
        printf("password : ");
        scanf("%31s", req.password);
        int c; while ((c = getchar()) != '\n' && c != EOF); // vider le buffer

        if(send_message(dS, REQ_AUTH, &req, sizeof(req)) < 0){
            perror("Erreur pour envoyer message");
            close(dS);
            return -1;
        }
        
        // Recevoir la réponse du serveur
        MsgHeader header;
        str_RepAuth rep;
        memset(&rep, 0, sizeof(rep));

        if(recv_message(dS, &header, &rep, sizeof(rep)) < 0){
            perror("Erreur pour recevoir la réponse");
            close(dS);
            return -1;
        }

        if(rep.resultat == STATUS_OK){
            printf("Authentification réussie pour '%s' !\n", rep.username);
            return 1;
        } else {
            printf("Erreur : le pseudo '%s' n'existe pas ou le mot de passe est incorrect.\n", rep.username);
            return -1;
        }   
    }
    else{
        printf("Commande inconnue : %s\n", m);
        return -1;   
    }
}

int traiter_requete_jeu(int dS, char *m){
    if(strcmp(m, "chercher partie") == 0){

        printf("en attente de l'adversaire...\n");
        if(send_message(dS, REQ_SEARCH, NULL, 0) < 0){
            perror("Erreur pour envoyer message");
            close(dS);
            return -1;
        }
        
        // Recevoir la réponse du serveur
        MsgHeader header;
        str_RepSearch rep;
        memset(&rep, 0, sizeof(rep));

        if(recv_message(dS, &header, &rep, sizeof(rep)) < 0){
            perror("Erreur pour recevoir la réponse");
            close(dS);
            return -1;
        }

        if(rep.resultat == STATUS_OK){
            printf("recherche réussie ! Vous jouez les %s.\n", rep.couleur == 1 ? "BLANCS" : "NOIRS");
            return rep.couleur; // on retourne la couleur pour la boucle de jeu
        } else {
            printf("Erreur : recherche échouée.\n");
            return -1;
        }   
    }

    else if (strcmp(m, "cherche partie sep") == 0) {
        printf("Recherche d'une partie en tant que spectateur...\n");

        if (send_message(dS, REQ_SEP, NULL, 0) < 0) {
            perror("Erreur envoi REQ_SEP");
            return -1;
        }

        MsgHeader header;
        str_RepSearch rep;
        memset(&rep, 0, sizeof(rep));

        if (recv_message(dS, &header, &rep, sizeof(rep)) < 0) {
            perror("Erreur pour recevoir la réponse spectateur");
            return -1;
        }

        if (rep.resultat == STATUS_OK) {
            printf("Mode spectateur activé sur la partie contre : %s\n", rep.adversaire);
            return -1; // Indique au moteur de jeu qu'on est spectateur
        } else {
            printf("Aucune partie en cours à observer.\n");
            return -2; // Code d'erreur spécifique pour éviter de lancer la boucle de jeu
        }
    }

    // Cas par défaut : commande inconnue ou échec
    printf("Commande inconnue ou erreur : %s\n", m);
    return -2;
}


int main(int argc, char *argv[]) {
    lancement_client(argc, argv);
    return 0;
}