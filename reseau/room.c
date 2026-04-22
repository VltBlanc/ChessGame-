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

// Initialise un slot de room (utilisé par room_creer et room_supprimer)
static void room_init_slot(Room *room) {
  room->joueurs[0] = -1;
  room->joueurs[1] = -1;
  for (int i = 0; i < 10; i++) {
    room->spectateurs[i] = -1;
  }
  room->nb_spectateurs = 0;
  room->etat = 0;
  room->trait = 0; // blanc commence
  room->threads_actifs = 0;

  pthread_mutex_init(&room->mutex, NULL);
  pthread_cond_init(&room->cond_prete, NULL);
}

int room_creer() {
  // Chercher un slot libre (room terminée ou supprimée) avant d'en allouer un nouveau
  for (int i = 0; i < nb_rooms; i++) {
    if (rooms[i].etat == -1) {
      // Réutiliser ce slot
      room_init_slot(&rooms[i]);
      printf("[room] Réutilisation du slot %d pour une nouvelle room\n", i);
      return i;
    }
  }

  // Pas de slot libre → en créer un nouveau
  if (nb_rooms >= MAX_ROOMS) {
    printf("[room] ERREUR : nombre maximum de rooms atteint (%d)\n", MAX_ROOMS);
    return -1;
  }

  int idx = nb_rooms;
  room_init_slot(&rooms[idx]);
  nb_rooms++;
  printf("[room] Nouvelle room créée : index=%d (total=%d)\n", idx, nb_rooms);
  return idx;
}

void room_supprimer(int room_idx) {
  if (room_idx < 0 || room_idx >= nb_rooms) return;

  Room *room = &rooms[room_idx];

  // Détruire les primitives de synchronisation
  pthread_mutex_destroy(&room->mutex);
  pthread_cond_destroy(&room->cond_prete);

  // Marquer le slot comme libre pour réutilisation
  room->etat = -1;
  room->joueurs[0] = -1;
  room->joueurs[1] = -1;
  for (int i = 0; i < 10; i++) {
    room->spectateurs[i] = -1;
  }
  room->nb_spectateurs = 0;
  room->threads_actifs = 0;

  printf("[room] Room %d supprimée et slot libéré\n", room_idx);
}

int room_rejoindre_spectateur(Room *room, int socket) {
  // protection
  pthread_mutex_lock(&room->mutex);

  if (room->nb_spectateurs < 10) {
    // Chercher le premier slot libre dans le tableau
    int slot = -1;
    for (int i = 0; i < 10; i++) {
      if (room->spectateurs[i] == -1) {
        slot = i;
        break;
      }
    }
    if (slot == -1) {
      // Tous les slots sont pris (ne devrait pas arriver si nb_spectateurs < 10)
      pthread_mutex_unlock(&room->mutex);
      printf("[room] Erreur : pas de slot spectateur libre !\n");
      return -1;
    }
    room->spectateurs[slot] = socket;
    room->nb_spectateurs++;
    room->threads_actifs++;

    printf("[room] Un spectateur a rejoint. Total : %d/10\n", room->nb_spectateurs);

    pthread_mutex_unlock(&room->mutex);
    return 0; // Succès
  }

  pthread_mutex_unlock(&room->mutex);
  printf("[room] Erreur : Room de spectateurs pleine !\n");
  return -1; // Échec
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

  // Chercher une room en attente (etat==0 avec joueur[0] rempli mais joueur[1] libre)
  // OU créer une nouvelle room
  int room_idx = -1;

  // D'abord, chercher une room en attente d'un 2ème joueur
  for (int i = 0; i < nb_rooms; i++) {
    if (rooms[i].etat == 0 && rooms[i].joueurs[0] != -1 && rooms[i].joueurs[1] == -1) {
      room_idx = i;
      break;
    }
  }

  if (room_idx == -1) {
    // Pas de room en attente → en créer une nouvelle
    room_idx = room_creer();
    if (room_idx < 0) {
      pthread_mutex_unlock(&mutex_rooms);
      return -1;
    }
  }

  Room *room = &rooms[room_idx];

  if (room->joueurs[0] == -1) {
    // Premier joueur (blanc) → il doit ATTENDRE le deuxième
    room->joueurs[0] = socket;
    strncpy(room->username_blanc, username, sizeof(room->username_blanc) - 1);
    room->username_blanc[sizeof(room->username_blanc) - 1] = '\0';
    room->threads_actifs++;
    printf("joueur 1 (%s) rejoint la room %d\n", username, room_idx);
    pthread_mutex_unlock(&mutex_rooms);

    // BLOQUE ici jusqu'à ce que joueur 2 arrive et mette etat >= 1
    pthread_mutex_lock(&room->mutex);
    while (room->etat < 1) {
      pthread_cond_wait(&room->cond_prete, &room->mutex);
    }
    pthread_mutex_unlock(&room->mutex);

    return room_idx;
  } else {
    // Deuxième joueur (noir) → room prête !
    room->joueurs[1] = socket;
    strncpy(room->username_noir, username, sizeof(room->username_noir) - 1);
    room->username_noir[sizeof(room->username_noir) - 1] = '\0';
    room->threads_actifs++;
    printf("joueur 2 (%s) rejoint la room %d\n", username, room_idx);

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
    return room_idx;
  }
}

// Fonction utilitaire : envoyer REP_RESULTAT_PARTIE à tous les spectateurs
static void notifier_spectateurs_fin(Room *room, str_RepResultatPartie *rep) {
  for (int i = 0; i < 10; i++) {
    if (room->spectateurs[i] != -1) {
      send_message(room->spectateurs[i], REP_RESULTAT_PARTIE, rep, sizeof(*rep));
    }
  }
}

// Décrémenter threads_actifs et supprimer la room si c'est le dernier thread
static void room_thread_sortie(Room *room, int room_idx) {
  pthread_mutex_lock(&mutex_rooms);
  room->threads_actifs--;
  int dernier = (room->threads_actifs <= 0);
  pthread_mutex_unlock(&mutex_rooms);

  if (dernier) {
    room_supprimer(room_idx);
  }
}

// Trouver l'index d'une room à partir de son pointeur
static int room_trouver_index(Room *room) {
  for (int i = 0; i < nb_rooms; i++) {
    if (&rooms[i] == room) return i;
  }
  return -1;
}

void room_boucle_jeu(Room *room, int mon_socket, int ma_couleur) {
  (void)ma_couleur;
  MsgHeader header;
  char buf[512];

  while (room->etat == 2) {
    // Attendre un message du client
    int type = recv_message(mon_socket, &header, buf, sizeof(buf));
    if (type <= 0 || header.type == REQ_RESIGN) {

      // Si c'est un SPECTATEUR
      if (ma_couleur == 3) {
        printf("[room] Un spectateur a quitté la room. Le match continue.\n");
        // On retire le spectateur de la liste pour libérer la place
        pthread_mutex_lock(&room->mutex);
        for (int i = 0; i < 10; i++) {
          if (room->spectateurs[i] == mon_socket) {
            room->spectateurs[i] = -1;
            room->nb_spectateurs--;
            break;
          }
        }
        pthread_mutex_unlock(&room->mutex);

        // Décrémenter threads_actifs et potentiellement nettoyer
        int idx = room_trouver_index(room);
        if (idx >= 0) room_thread_sortie(room, idx);
        return; // On sort de la fonction pour ce thread UNIQUEMENT
      }

      // Si c'est un JOUEUR, alors on arrête la partie
      printf("[room] Déconnexion d'un joueur ou abandon. Fin du match.\n");
      pthread_mutex_lock(&room->mutex);
      if (room->etat == 2) {
        int mon_index = (room->joueurs[0] == mon_socket) ? 0 : 1;
        int adversaire = room->joueurs[mon_index == 0 ? 1 : 0];

        partie_resultat res = (mon_index == 0) ? RESULTAT_PARTIE_NOIR_WIN : RESULTAT_PARTIE_WHITE_WIN;
        historique_finaliser(&room->hist, res, GAME_RESIGN);

        str_RepResultatPartie rep;
        memset(&rep, 0, sizeof(rep));
        rep.raison = GAME_RESIGN;
        rep.gagnant = mon_index == 0 ? 0 : 1;

        // Notifier l'adversaire
        if (adversaire != -1) {
          send_message(adversaire, REP_RESULTAT_PARTIE, &rep, sizeof(rep));
        }

        // Notifier tous les spectateurs
        notifier_spectateurs_fin(room, &rep);

        room->etat = 3; // TERMINEE
      }
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
      // Envoyer aussi aux spectateurs
      for (int i = 0; i < 10; i++) {
        if (room->spectateurs[i] != -1) {
          send_message(room->spectateurs[i], REP_MOVE, &rep_move, sizeof(rep_move));
        }
      }

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

        // Préparer le message de résultat pour l'adversaire et les spectateurs
        str_RepResultatPartie rep;
        memset(&rep, 0, sizeof(rep));
        rep.raison = req_go->raison;
        rep.gagnant = req_go->gagnant;

        // Notifier l'adversaire
        int mon_index = (room->joueurs[0] == mon_socket) ? 0 : 1;
        int adversaire = room->joueurs[mon_index == 0 ? 1 : 0];
        if (adversaire != -1) {
          send_message(adversaire, REP_RESULTAT_PARTIE, &rep, sizeof(rep));
        }

        // Notifier tous les spectateurs
        notifier_spectateurs_fin(room, &rep);

        room->etat = 3; // TERMINEE
      }
      pthread_mutex_unlock(&room->mutex);
      break;

    } else {
      printf("[room] type de message inattendu : %d\n", header.type);
    }
  }

  // Décrémenter et potentiellement supprimer la room
  int idx = room_trouver_index(room);
  if (idx >= 0) room_thread_sortie(room, idx);
}
