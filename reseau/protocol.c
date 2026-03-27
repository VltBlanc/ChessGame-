#include <sys/socket.h>
#include "chess.h"
#include "protocol.h"

// Envoie la structure move_t telle quelle au serveur
int network_send_move(int fd, move_t move) {
    return send(fd, &move, sizeof(move_t), 0);
}

// Reçoit la structure move_t depuis le serveur
int network_receive_move(int fd, move_t *move) {
    // Note: Dans un vrai jeu, il faudrait rendre cette socket 
    // NON-BLOQUANTE pour ne pas figer l'image.
    int n = recv(fd, move, sizeof(move_t), MSG_DONTWAIT); 
    if (n == sizeof(move_t)) return 0; // Succès
    return -1; // Rien reçu encore
}