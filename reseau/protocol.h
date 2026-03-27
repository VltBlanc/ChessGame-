#ifndef NETWORK_H
#define NETWORK_H

#include "chess.h"

// Initialise la connexion
int network_connect(const char *ip, int port);
// Envoie un coup move_t
int network_send_move(int socket, move_t move);
// Attend et reçoit un coup move_t du serveur
int network_receive_move(int socket, move_t *move);

#endif