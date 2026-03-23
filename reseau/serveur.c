#include <stdio.h>       
#include <stdlib.h>      
#include <string.h>      
#include <unistd.h>      
#include <sys/types.h>   
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <pthread.h>

// Fonction exécutée par chaque thread
void* routine_client(void* arg) {
    int dSC = *(int*)arg; 
    free(arg);

    //communication
    char msg [20];
    memset(msg, 0, sizeof(msg));
    if(recv(dSC, msg, sizeof(msg)-1, 0)<0){
        perror("Erreur pour recevoir message\n");
        close(dSC);
        pthread_exit(0);
    }
    printf("recu : %s\n", msg);

    int r=-1;
    if(strcmp(msg, "salut") == 0){
        r = 1;
    }

    if(send(dSC, &r, sizeof(int), 0)<0){
        perror("Erreur pour envoyer message\n");
        close(dSC);
        pthread_exit(0);
    }

    //ferme socket
    close (dSC);

    //arrete le thread
    pthread_exit(0);
}

// pour lancer
// ./serveur numero_du_port
int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Erreur : Tu dois préciser un port ! Exemple : %s 8080\n", argv[0]);
        exit(1);
    }
    //creation socket
    int dS= socket(PF_INET, SOCK_STREAM, 0);
    if (dS<0){
        perror("Erreur création socket serveur\n");
        exit(1);
    }

    //on va utiliser IPV4
    struct sockaddr_in ad;
    ad.sin_family = AF_INET;

    //adresse d'écoute
    ad.sin_addr.s_addr = INADDR_ANY;

    //convertie le port
    ad.sin_port = htons(atoi(argv[1]));

    //nommer socket
    if(bind(dS, (struct sockaddr*)&ad, sizeof(ad))<0){
        perror("Erreur nommage socket (surement port deja utilisé)\n");
        close(dS);
        exit(1);
    }

    //socket en mode ecoute
    if(listen(dS, 7)<0){
        perror("Erreur pour passer la socket en mode ecoute\n");
        close(dS);
        exit(1);
    }


    while (1){
        struct sockaddr_in aC;
        socklen_t lg = sizeof(struct sockaddr_in);
        //accepter connexion
        int dSC= accept(dS, (struct sockaddr*) &aC,&lg);
        if (dSC < 0) {
            perror("erreur sur l'acceptation de la connexion\n");
            continue;
        }
        
        // On alloue dynamiquement pour éviter que dSC ne change avant que le thread ne le lise
        int* client_ptr = malloc(sizeof(int));
        if(client_ptr==NULL){
            perror("erreur sur le malloc\n");
            close(dSC);
            continue;
        }
        *client_ptr = dSC;

        //creation du thread
        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, routine_client, client_ptr)!=0){
            perror("Erreur pour creer le pthread\n");
            free(client_ptr);
            close(dSC);
            continue;
        }

        //fait en sorte que le thread ne soit plus en mode join (et donc quand il se finit il se supprime vraiment)
        pthread_detach(thread_id);
    }

    //ferme socket
    close (dS);

    return 0;
}