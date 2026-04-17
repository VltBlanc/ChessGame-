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

    memset(m, 0, sizeof(m));
    printf("commande disponible : chercher partie\n");
    printf("Entrer une commande : ");
    fgets(m, sizeof(m), stdin);
    m[strcspn(m, "\n")] = '\0';

    int couleur = traiter_requete_jeu(dS, m); 
    if (couleur >= 0) {
        client_boucle_jeu(dS, couleur);
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
    else{
        printf("Commande inconnue : %s\n", m);
        return -1;
    }
}

void client_boucle_jeu(int dS, int ma_couleur) {
    int trait = 1; // 1 = blanc, 0 = noir (les blancs commencent)
    MsgHeader header;
    char buf[512];

    printf("\n--- DÉBUT DE LA PARTIE ---\n");

    while (1) {
        if (trait == ma_couleur) {
            // --- C'EST NOTRE TOUR ---
            printf("\nÀ vous de jouer ! Entrez un coup (ex: e2e4) ou 'quit' : ");
            char coup[20];
            memset(coup, 0, sizeof(coup));
            fgets(coup, sizeof(coup), stdin);
            coup[strcspn(coup, "\n")] = '\0';

            if (strcmp(coup, "quit") == 0) {
                printf("Vous avez quitté la partie.\n");
                break;
            }

            // Simplification temporaire du parsage (ex : 1,2 vers 3,4 au lieu de format d'echecs e2e4 pour le moment)
            // L'idéal est de brancher cela avec ta librairie 'chess.c'.
            // On hardcode juste l'envoi pour l'architecture réseau ici:
            str_ReqMove req;
            memset(&req, 0, sizeof(req));
            if (strlen(coup) >= 4) {
                 req.dep_col = coup[0] - 'a';
                 req.dep_ligne = coup[1] - '1';
                 req.vers_col = coup[2] - 'a';
                 req.vers_ligne = coup[3] - '1';
            }

            if (send_message(dS, REQ_MOVE, &req, sizeof(req)) < 0) {
                perror("Erreur envoi coup");
                break;
            }

            // Attendre la réponse du serveur (ACK)
            int type = recv_message(dS, &header, buf, sizeof(buf));
            if (type <= 0) {
                printf("Déconnecté du serveur.\n");
                break;
            }

            if (header.type == REP_MOVE_ACK) {
                str_RepMove *rep = (str_RepMove *)buf;
                if (rep->resultat == STATUS_OK) {
                    printf("Coup accepté par le serveur.\n");
                    trait = (trait == 1) ? 0 : 1; // On passe la main
                } else {
                    printf("Coup refusé (invalide ou pas ton tour), réessayez.\n");
                }
            } else if (header.type == REP_RESULTAT_PARTIE) {
                str_RepResultatPartie *rep = (str_RepResultatPartie *)buf;
                if (rep->raison == GAME_RESIGN) {
                    printf("\nL'adversaire a quitté la partie. Vous avez gagné par abandon.\n");
                } else {
                    printf("\nLa partie est terminée.\n");
                }
                break;
            } else {
                printf("Réponse inattendue au lieu de ACK: %d\n", header.type);
            }
        } 
        else {
            // --- TOUR DE L'ADVERSAIRE ---
            printf("\nEn attente du coup de l'adversaire...\n");

            int type = recv_message(dS, &header, buf, sizeof(buf));
            if (type <= 0) {
                printf("Déconnecté du serveur.\n");
                break;
            }

            if (header.type == REP_MOVE) {
                str_RepMove *rep = (str_RepMove *)buf;
                printf("L'adversaire a joué : de (%d,%d) vers (%d,%d)\n", 
                        rep->plateau[0], rep->plateau[1], rep->plateau[2], rep->plateau[3]);
                
                trait = ma_couleur; // C'est de nouveau à nous
            } else if (header.type == REP_RESULTAT_PARTIE) {
                str_RepResultatPartie *rep = (str_RepResultatPartie *)buf;
                if (rep->raison == GAME_RESIGN) {
                    printf("\nL'adversaire a quitté la partie. Vous avez gagné par abandon.\n");
                } else {
                    printf("\nLa partie est terminée.\n");
                }
                break;
            } else {
                 printf("Message inattendu : %d\n", header.type);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    lancement_client(argc, argv);
    return 0;
}