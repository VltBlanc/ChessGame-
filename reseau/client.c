#include <stdio.h>       
#include <stdlib.h>      
#include <string.h>      
#include <unistd.h>      
#include <sys/types.h>   
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h> 

// pour lancer :
// ./client IP_serveur numero_du_port
// si serveur sur le meme pc IP=127.0.0.1
int main(int argc, char *argv[]) {

    if (argc < 3) {
        perror("Erreur : Tu dois préciser l'ip et le port du serveur");
        exit(1);
    }

    //creation socket
    int dS= socket(PF_INET, SOCK_STREAM, 0);
    if (dS<0){
        perror("Erreur création socket client");
        exit(1);
    }

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
    printf("Entrer une commande : ");
    scanf("%[^\n]", m);
    if(send(dS, m, strlen(m) + 1, 0)<0){
        perror("Erreur pour envoyer message");
        close(dS);
        exit(1);
    }

    int r;
    if(recv(dS, &r, sizeof(int), 0)<0){
        perror("Erreur pour recevoir message");
        close(dS);
        exit(1);
    }
    printf("reponse : %d\n", r);

    //fermer socket
    close (dS);

    return 0;
}