#include "chat.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

void diffuser_chat_room(Room *room, str_RepChat *msg, int socket_emetteur) {
  if (room == NULL || msg == NULL) return;

  // Préparer le header + payload à envoyer
  MsgHeader header;
  header.type = REP_CHAT;
  header.length = sizeof(str_RepChat);

  // Envoyer aux joueurs
  for (int i = 0; i < 2; i++) {
    if (room->joueurs[i] != -1 && room->joueurs[i] != socket_emetteur) {
      // Envoi non-bloquant : on ignore les erreurs pour ne pas bloquer le serveur
      send(room->joueurs[i], &header, sizeof(MsgHeader), MSG_DONTWAIT);
      send(room->joueurs[i], msg, sizeof(str_RepChat), MSG_DONTWAIT);
    }
  }

  // Envoyer aux spectateurs
  for (int i = 0; i < 10; i++) {
    if (room->spectateurs[i] != -1 && room->spectateurs[i] != socket_emetteur) {
      send(room->spectateurs[i], &header, sizeof(MsgHeader), MSG_DONTWAIT);
      send(room->spectateurs[i], msg, sizeof(str_RepChat), MSG_DONTWAIT);
    }
  }
}