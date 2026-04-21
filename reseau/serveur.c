#define _POSIX_C_SOURCE 200809L
#include "serveur.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Socket serveur globale pour le nettoyage lors d'un signal
static volatile int g_dS = -1;

// Gestionnaire de signal pour fermeture propre (Ctrl+C, kill, etc.)
// Note : exit() ferme automatiquement toutes les sockets clients ouvertes
void handler_signal(int sig) {
  (void)sig;
  const char msg[] = "\n[serveur] Signal recu, fermeture propre...\n";
  write(STDOUT_FILENO, msg, sizeof(msg) - 1);

  // Fermer la socket serveur pour débloquer accept()
  if (g_dS != -1) {
    close(g_dS);
    g_dS = -1;
  }

  // exit() fermera les sockets clients automatiquement
  _exit(0);
}

// Fonction exécutée par chaque thread
void *routine_client(void *arg) {
  int dSC = *(int *)arg;
  free(arg);

  // Lire le header + payload via recv_message
  MsgHeader header;
  char buf[512];
  memset(buf, 0, sizeof(buf));

  char username[32] = {0}; // Pseudo du joueur authentifié

  int login = -1;
  while (login != 5) {
    int type = recv_message(dSC, &header, buf, sizeof(buf));
    if (type < 0) {
      perror("Erreur pour recevoir message");
      close(dSC);
      pthread_exit(0);
    }

    printf("recu : type=%d, length=%u\n", header.type, header.length);

    login = traiter_requete_debut(dSC, &header, buf);

    // Récupérer le pseudo après authentification réussie
    if (login == 5) {
      str_RequAuth *req = (str_RequAuth *)buf;
      strncpy(username, req->username, sizeof(username) - 1);
      username[sizeof(username) - 1] = '\0';
      printf("[serveur] Joueur authentifié : %s\n", username);
    }
  }

  while (1) {
    int type = recv_message(dSC, &header, buf, sizeof(buf));
    if (type <= 0) {
      printf("[serveur] Deconnexion joueur %s.\n", username);
      break;
    }

    printf("recu : type=%d, length=%u\n", header.type, header.length);

    traiter_requete_jeu(dSC, &header, buf, username);
  }

  // ferme socket
  close(dSC);
  pthread_exit(0);
}

int traiter_requete_debut(int dSC, MsgHeader *header, void *payload) {

  // creer compte
  if (header->type == REQ_CONNEXION) {
    str_RequAuth *req = (str_RequAuth *)payload;

    printf("[creer compte] username='%s'\n", req->username);

    printf("password='%s'\n", req->password);
    printf("username='%s'\n", req->username);
    int ok = logique_create_account(req->username, req->password);

    str_RepAuth rep;
    memset(&rep, 0, sizeof(rep));
    rep.resultat = ok ? STATUS_OK : STATUS_ERR_AUTH;
    strncpy(rep.username, req->username, sizeof(rep.username) - 1);

    if (send_message(dSC, REP_CONNEXION, &rep, sizeof(rep)) < 0) {
      perror("Erreur pour envoyer REP_CONNEXION");
      return -1;
    }
    return ok ? STATUS_OK : STATUS_ERR_AUTH;
  }

  // connexion
  else if (header->type == REQ_AUTH) {
    str_RequAuth *req = (str_RequAuth *)payload;

    printf("[connexion] username='%s'\n", req->username);

    printf("password='%s'\n", req->password);
    printf("username='%s'\n", req->username);
    int ok = logique_connexion(req->username, req->password);

    str_RepAuth rep;
    memset(&rep, 0, sizeof(rep));
    rep.resultat = ok ? STATUS_OK : STATUS_ERR_AUTH;
    strncpy(rep.username, req->username, sizeof(rep.username) - 1);

    if (send_message(dSC, REP_AUTH, &rep, sizeof(rep)) < 0) {
      perror("Erreur pour envoyer REP_AUTH");
      return -1;
    }
    return ok ? 5 : STATUS_ERR_AUTH;
  } else {
    printf("[serveur] type de requête inconnu : %d\n", header->type);
    return -1;
  }
}

int traiter_requete_jeu(int dSC, MsgHeader *header, void *payload,
                        const char *username) {
  (void)payload;

  // chercher partie
  if (header->type == REQ_SEARCH) {
    printf("[serveur] REQ_SEARCH reçu de %s, mise en file d'attente...\n",
           username);

    // Bloque ici tant que les 2 joueurs ne sont pas réunis
    int room_idx = room_rejoindre_file(dSC, username);

    // On arrive ici SEULEMENT quand les 2 joueurs sont dans la room
    Room *room = &rooms[room_idx];
    int couleur = (room->joueurs[0] == dSC) ? 1 : 0; // 1=blanc, 0=noir

    str_RepSearch rep;
    memset(&rep, 0, sizeof(rep));
    rep.resultat = STATUS_OK;
    rep.couleur = couleur;

    if (send_message(dSC, REP_SEARCH, &rep, sizeof(rep)) < 0) {
      perror("Erreur pour envoyer REP_SEARCH");
      close(dSC);
      pthread_exit(0);
    }
    printf("[serveur] Match trouvé ! room=%d, %s joue %s\n", room_idx, username,
           couleur ? "blanc" : "noir");

    // Phase 3 : Boucle de jeu — relayer les coups
    room_boucle_jeu(room, dSC, couleur);
    return 0;
  }
  else if (header->type == REQ_SEP) {
    // Le payload contient l'ID de la room (envoyé par le spectateur)
    int room_voulue = *(int*)payload;

    pthread_mutex_lock(&mutex_rooms);
    // Vérification de validité
    if (room_voulue >= 0 && room_voulue < nb_rooms && rooms[room_voulue].etat == 2) {
      pthread_mutex_unlock(&mutex_rooms);

      int res = room_rejoindre_spectateur(&rooms[room_voulue], dSC);

      str_RepSearch rep;
      memset(&rep, 0, sizeof(rep));

      if (res == 0) {
        rep.resultat = STATUS_OK;
        rep.couleur = 3; // Mode spectateur
        strncpy(rep.adversaire, rooms[room_voulue].username_blanc, 31);
        send_message(dSC, REP_SEARCH, &rep, sizeof(rep));
        room_boucle_jeu(&rooms[room_voulue], dSC, 3);
      } else {
        rep.resultat = STATUS_ERR_AUTH; // Room pleine
        send_message(dSC, REP_SEARCH, &rep, sizeof(rep));
      }
    } else {
      pthread_mutex_unlock(&mutex_rooms);
      // Room invalide ou non commencée
      str_RepSearch rep;
      rep.resultat = STATUS_ERR_AUTH;
      send_message(dSC, REP_SEARCH, &rep, sizeof(rep));
    }
    return 0;
  }
  else if (header->type == REQ_LIST_ROOMS) {
    str_RepSimpleList rep;
    memset(&rep, 0, sizeof(rep));

    pthread_mutex_lock(&mutex_rooms);
    // On parcourt toutes les rooms créées
    for (int i = 0; i < nb_rooms; i++) {
      rep.indices[i] = i; // On stocke l'indice
    }
    rep.nb_actives = nb_rooms; // On indique combien il y en a
    pthread_mutex_unlock(&mutex_rooms);

    send_message(dSC, REP_LIST_ROOMS, &rep, sizeof(rep));
    return 0;
  }
  else if (header->type == REQ_HISTORIQUE) {
    str_RepHistorique rep;
    memset(&rep, 0, sizeof(rep));
    rep.resultat = STATUS_OK;
    strncpy(rep.username, username, sizeof(rep.username) - 1);
    strncpy(rep.historique, historique_afficher_profil(username),
            sizeof(rep.historique) - 1);
    if (send_message(dSC, REP_HISTORIQUE, &rep, sizeof(rep)) < 0) {
      perror("Erreur pour envoyer REP_HISTORIQUE");
      close(dSC);
      pthread_exit(0);
    }
    return 0;
  } 
  else if (header->type == REQ_HISTORIQUE_PARTIE) {
    str_RequHistorique *req = (str_RequHistorique *)payload;
    str_RepHistorique rep;
    memset(&rep, 0, sizeof(rep));
    rep.resultat = STATUS_OK;
    strncpy(rep.username, username, sizeof(rep.username) - 1);
    strncpy(rep.historique, historique_exporter_pgn(req->id_partie),
            sizeof(rep.historique) - 1);
    if (send_message(dSC, REP_HISTORIQUE_PARTIE, &rep, sizeof(rep)) < 0) {
      perror("Erreur pour envoyer REP_HISTORIQUE_PARTIE");
      close(dSC);
      pthread_exit(0);
    }
    return 0;
  } else {
    printf("[serveur] type de requête inconnu : %d\n", header->type);
    return -1;
  }
}

// pour lancer
// ./serveur numero_du_port
int lancement_serveur(int argc, char *argv[]) {

  if (argc < 2) {
    printf("Erreur : Tu dois préciser un port ! Exemple : %s 8080\n", argv[0]);
    exit(1);
  }

  // Enregistrement des gestionnaires de signaux
  struct sigaction sa;
  sa.sa_handler = handler_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);  // Ctrl+C
  sigaction(SIGTERM, &sa, NULL); // kill

  // creation socket
  int dS = socket(PF_INET, SOCK_STREAM, 0);
  if (dS < 0) {
    perror("Erreur création socket serveur\n");
    exit(1);
  }
  g_dS = dS; // sauvegarder pour le handler de signal

  // Permet de réutiliser le port immédiatement après un arrêt
  int opt = 1;
  setsockopt(dS, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // on va utiliser IPV4
  struct sockaddr_in ad;
  ad.sin_family = AF_INET;

  // adresse d'écoute
  ad.sin_addr.s_addr = INADDR_ANY;

  // convertie le port
  ad.sin_port = htons(atoi(argv[1]));

  // nommer socket
  if (bind(dS, (struct sockaddr *)&ad, sizeof(ad)) < 0) {
    perror("Erreur nommage socket (surement port deja utilisé)\n");
    close(dS);
    exit(1);
  }

  // socket en mode ecoute
  if (listen(dS, 7) < 0) {
    perror("Erreur pour passer la socket en mode ecoute\n");
    close(dS);
    exit(1);
  }
  printf("[serveur] Serveur lancé sur le port %s\n", argv[1]);

  while (1) {
    struct sockaddr_in aC;
    socklen_t lg = sizeof(struct sockaddr_in);
    // accepter connexion
    int dSC = accept(dS, (struct sockaddr *)&aC, &lg);
    if (dSC < 0) {
      perror("erreur sur l'acceptation de la connexion\n");
      continue;
    }

    // On alloue dynamiquement pour éviter que dSC ne change avant que le thread
    // ne le lise
    int *client_ptr = malloc(sizeof(int));
    if (client_ptr == NULL) {
      perror("erreur sur le malloc\n");
      close(dSC);
      continue;
    }
    *client_ptr = dSC;

    // creation du thread
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, routine_client, client_ptr) != 0) {
      perror("Erreur pour creer le pthread\n");
      free(client_ptr);
      close(dSC);
      continue;
    }

    // fait en sorte que le thread ne soit plus en mode join (et donc quand il
    // se finit il se supprime vraiment)
    pthread_detach(thread_id);
  }

  // ferme socket
  close(dS);

  return 0;
}

int main(int argc, char *argv[]) {
  lancement_serveur(argc, argv);
  return 0;
}