#include "room.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_ROOMS 100
Room rooms[MAX_ROOMS];
int nb_rooms = 0;
pthread_mutex_t mutex_rooms = PTHREAD_MUTEX_INITIALIZER;

int room_creer() {
  Room room;
  room.joueurs[0] = -1;
  room.joueurs[1] = -1;
  room.spectateurs[0] = -1;
  room.nb_spectateurs = 0; // max 10
  room.etat = 0;
  room.trait = 0; // blanc commence

  // Initialiser le mutex et la condition de cette room
  pthread_mutex_init(&room.mutex, NULL);
  pthread_cond_init(&room.cond_prete, NULL);

  rooms[nb_rooms] = room;
  nb_rooms++;
  printf("nouvelle room créée : %d\n", nb_rooms);
  return 0;
}

// surement a modif(gérer le mutex?)
int room_rejoindre_spectateur(Room *room, int socket) {
  if (room->nb_spectateurs < 10) {
    room->spectateurs[room->nb_spectateurs] = socket;
    room->nb_spectateurs++;
    printf("spectateur rejoint la room %d\n", nb_rooms);
    return 0;
  }
  return -1;
}

int room_demarre(Room *room) {
  if (room->etat == 1) {
    room->etat = 2;
    room->trait = 0; // joueur[0] (blanc) commence
    printf("room en jeu\n");
    return 0;
  }
  return -1;
}

int room_rejoindre_file(int socket, const char *username) {
  pthread_mutex_lock(&mutex_rooms);

  if (nb_rooms == 0 || rooms[nb_rooms - 1].joueurs[1] != -1) {
    // Aucune room ou la dernière est pleine → en créer une nouvelle
    room_creer();
  }
  Room *room = &rooms[nb_rooms - 1];

  if (room->joueurs[0] == -1) {
    // Premier joueur (blanc) → il doit ATTENDRE le deuxième
    room->joueurs[0] = socket;
    strncpy(room->username_blanc, username, sizeof(room->username_blanc) - 1);
    room->username_blanc[sizeof(room->username_blanc) - 1] = '\0';
    printf("joueur 1 (%s) rejoint la room %d\n", username, nb_rooms);
    pthread_mutex_unlock(&mutex_rooms);

    // BLOQUE ici jusqu'à ce que joueur 2 arrive et mette etat >= 1
    pthread_mutex_lock(&room->mutex);
    while (room->etat < 1) {
      pthread_cond_wait(&room->cond_prete, &room->mutex);
    }
    pthread_mutex_unlock(&room->mutex);

    return nb_rooms - 1;
  } else {
    // Deuxième joueur (noir) → room prête !
    room->joueurs[1] = socket;
    strncpy(room->username_noir, username, sizeof(room->username_noir) - 1);
    room->username_noir[sizeof(room->username_noir) - 1] = '\0';
    printf("joueur 2 (%s) rejoint la room %d\n", username, nb_rooms);

    // Initialiser l'historique maintenant qu'on a les deux joueurs
    historique_init(&room->hist, room->username_blanc, room->username_noir);
    printf("[historique] Partie %ld initialisée : %s vs %s\n",
           room->hist.identifiant_partie, room->username_blanc,
           room->username_noir);

    // Signaler au joueur 1 que la room est prête
    pthread_mutex_lock(&room->mutex);
    room->etat = 1;
    room_demarre(room);                     // passe etat à 2 (EN_JEU)
    pthread_cond_signal(&room->cond_prete); // Réveille le joueur 1
    pthread_mutex_unlock(&room->mutex);

    pthread_mutex_unlock(&mutex_rooms);
    return nb_rooms - 1;
  }
}

void room_boucle_jeu(Room *room, int mon_socket, int ma_couleur) {
  (void)ma_couleur;
  MsgHeader header;
  char buf[512];

  while (room->etat == 2) {
    // Attendre un message du client
    int type = recv_message(mon_socket, &header, buf, sizeof(buf));
    if (type <= 0 || header.type == REQ_RESIGN) {
      printf("[room] déconnexion d'un joueur ou abandon\n");

      pthread_mutex_lock(&room->mutex);
      if (room->etat == 2) {
        int mon_index = (room->joueurs[0] == mon_socket) ? 0 : 1;
        int adversaire = room->joueurs[mon_index == 0 ? 1 : 0];

        // Finaliser l'historique : celui qui abandonne perd
        partie_resultat res = (mon_index == 0) ? RESULTAT_PARTIE_NOIR_WIN
                                               : RESULTAT_PARTIE_WHITE_WIN;
        historique_finaliser(&room->hist, res, GAME_RESIGN);

        if (adversaire != -1) {
          str_RepResultatPartie rep;
          memset(&rep, 0, sizeof(rep));
          rep.raison = GAME_RESIGN;
          rep.gagnant = mon_index == 0 ? 0 : 1; // 0=noir gagne, 1=blanc gagne
          send_message(adversaire, REP_RESULTAT_PARTIE, &rep, sizeof(rep));
        }
      }
      room->etat = 3; // TERMINEE
      pthread_mutex_unlock(&room->mutex);
      break;
    }

    if (header.type == REQ_MOVE) {
      str_ReqMove *req = (str_ReqMove *)buf;

      // Vérifier que c'est bien le tour de ce joueur
      int mon_index = (room->joueurs[0] == mon_socket) ? 0 : 1;
      if (room->trait != mon_index) {
        // Ce n'est pas son tour → refuser le coup
        str_RepMove rep_move;
        memset(&rep_move, 0, sizeof(rep_move));
        rep_move.resultat = STATUS_ERR_MOVE;
        send_message(mon_socket, REP_MOVE_ACK, &rep_move, sizeof(rep_move));
        continue;
      }

      // Enregistrer le coup dans l'historique
      move_t coup;
      memset(&coup, 0, sizeof(coup));
      coup.start.file = req->dep_col;
      coup.start.rank = req->dep_ligne;
      coup.end.file = req->vers_col;
      coup.end.rank = req->vers_ligne;
      historique_enregistrer_coup(coup, &room->hist);

      // Coup accepté → envoyer ACK au joueur qui a joué
      str_RepMove rep_ack;
      memset(&rep_ack, 0, sizeof(rep_ack));
      rep_ack.resultat = STATUS_OK;
      send_message(mon_socket, REP_MOVE_ACK, &rep_ack, sizeof(rep_ack));

      // Relayer le coup à l'adversaire
      int adversaire = room->joueurs[mon_index == 0 ? 1 : 0];

      str_RepMove rep_move;
      memset(&rep_move, 0, sizeof(rep_move));
      rep_move.resultat = STATUS_OK;
      rep_move.trait = (room->trait == 0) ? 1 : 0; // le trait passe à l'autre

      // On envoie les coordonnées du coup pour que le client puisse l'appliquer
      rep_move.dep_col = req->dep_col;
      rep_move.dep_ligne = req->dep_ligne;
      rep_move.vers_col = req->vers_col;
      rep_move.vers_ligne = req->vers_ligne;

      send_message(adversaire, REP_MOVE, &rep_move, sizeof(rep_move));

      // Changer le trait
      room->trait = (room->trait == 0) ? 1 : 0;

    } else if (header.type == REQ_GAME_OVER) {
      // Le client signale la fin de partie (mat, pat, nulle)
      str_ReqGameOver *req_go = (str_ReqGameOver *)buf;
      printf("[room] REQ_GAME_OVER reçu : raison=%d, gagnant=%d\n",
             req_go->raison, req_go->gagnant);

      pthread_mutex_lock(&room->mutex);
      if (room->etat == 2) {
        partie_resultat res;
        if (req_go->gagnant == 1)
          res = RESULTAT_PARTIE_WHITE_WIN;
        else if (req_go->gagnant == 0)
          res = RESULTAT_PARTIE_NOIR_WIN;
        else
          res = RESULTAT_PARTIE_DRAW;

        historique_finaliser(&room->hist, res, req_go->raison);
        room->etat = 3; // TERMINEE
      }
      pthread_mutex_unlock(&room->mutex);
      break;

    } else {
      printf("[room] type de message inattendu : %d\n", header.type);
    }
  }
}
