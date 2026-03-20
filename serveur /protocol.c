#include <stdio.h> // print, scanf etc.. 
#include <stdlib.h> // Pour les malloc, free etc... 
#include <string.h> // pour les manipulation des chaines caractères 
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h> 

#include "protocol.h"
#include <sys/socket.h>
#include <unistd.h>



int send_tout_message (int descripteur, void *buf,size_t len  ) {
    size_t envoye = 0
    while (envoye < len ) {
        size_t req = send( descripteur, (char*)buf + envoye, len - envoye ,0 ); // renvoie le nombre d'octets réellement enovoyé
        envoye += req; 
    
        if req <= 0 { // socket fermée ou erreur 
            return -1; 
        }
    }
    return 0; 
}



int recv_tout_message(int sock, void *buf, size_t len) {
    size_t received = 0;
    while (received < len) {
        ssize_t n = recv(sock, (char *)buf + received, len - received, 0);
        if (n <= 0) return -1;   // idem que fonction précédente 
        received += n;
    }
    return 0;
}




//Dictionnaire  des messages permetant de communiquer entre le Serveur et le Client 
//Commnet envoyer et recevoir les message

//ssize_t recv(int descripteur, const void * msg, size_t lg, int flags?  )
// ssize_t send(int descripteur, cosnt void *msg, size_t lg, int flags)



//Exemple j1 veut se connecter 
//||Créer une structure pour s'enregistrer 
//|| envoyer cette structure au serveur 


//MsgType type : permettra de déterminer quel type de message ils recoivnet (req_auth, req_move etc..)
//payload : pointeur vers les données à envoyer 
int send_message(int descripteur, MsgType type, void *payload, uint32_t size) {

}

int recv_message(int descripteur, MsgHeader *header, void *buf, uint32_t buf_size) {

}
