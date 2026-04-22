// Chat par room : broadcast des messages aux joueurs et spectateurs

#ifndef CHESS_CHAT_H
#define CHESS_CHAT_H

#include "room.h"
#include "protocol.h"

// Diffuse un message chat à tous les membres de la room (joueurs + spectateurs)
// sauf l'émetteur. Utilise send_message avec MSG_DONTWAIT pour ne pas bloquer.
void diffuser_chat_room(Room *room, str_RepChat *msg, int socket_emetteur);

#endif // CHESS_CHAT_H