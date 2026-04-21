#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "historique.h"

#define STATS_PATH "data/stats_joueurs.csv"
#define STATS_MAX_JOUEURS 4096

void notation_algebrique(move_t move, char *buffer) {
  char start_col = 'a' + move.start.file;
  char end_col = 'a' + move.end.file;
  int start_row = 8 - move.start.rank;
  int end_row = 8 - move.end.rank;

  if (move.promotion != ' ' && move.promotion != '\0') {
    sprintf(buffer, "%c%d%c%d%c", start_col, start_row, end_col, end_row,
            move.promotion);
  } else {
    sprintf(buffer, "%c%d%c%d", start_col, start_row, end_col, end_row);
  }
}

void historique_ajouter_index(historique_partie *h) {
  FILE *f = fopen("data/index_parties.csv", "a");
  if (f == NULL) {
    perror("Erreur ouverture index");
    return;
  }

  // On écrit : ID, Blanc, Noir, Resultat
  fprintf(f, "%ld,%s,%s,%d\n", h->identifiant_partie, h->joueur_blanc,
          h->joueur_noir, h->resultat);

  fclose(f);
}

long *historique_lister_parties_joueur(const char *pseudo, int *nb_parties) {
  FILE *f = fopen("data/index_parties.csv", "r");
  if (!f) {
    *nb_parties = 0;
    return NULL;
  }

  // Première passe : compter combien de parties appartiennent au joueur
  char ligne[256];
  int compteur = 0;
  while (fgets(ligne, sizeof(ligne), f)) {
    char *id_str = strtok(ligne, ",");
    char *blanc = strtok(NULL, ",");
    char *noir = strtok(NULL, ",");
    if (id_str && blanc && noir) {
      blanc[strcspn(blanc, "\n")] = '\0';
      noir[strcspn(noir, "\n")] = '\0';
      if (strcmp(pseudo, blanc) == 0 || strcmp(pseudo, noir) == 0)
        compteur++;
    }
  }

  // Allouer exactement la bonne taille
  long *ids = malloc(compteur * sizeof(long));
  if (!ids) {
    fclose(f);
    *nb_parties = 0;
    return NULL;
  }

  // Deuxième passe : remplir le tableau
  rewind(f); // Retour au début du fichier
  int i = 0;
  while (fgets(ligne, sizeof(ligne), f)) {
    char *id_str = strtok(ligne, ",");
    char *blanc = strtok(NULL, ",");
    char *noir = strtok(NULL, ",");
    if (id_str && blanc && noir) {
      blanc[strcspn(blanc, "\n")] = '\0';
      noir[strcspn(noir, "\n")] = '\0';
      if (strcmp(pseudo, blanc) == 0 || strcmp(pseudo, noir) == 0)
        ids[i++] = atol(id_str);
    }
  }

  fclose(f);
  *nb_parties = compteur;
  return ids;
}

void historique_init(historique_partie *h, char *playerBlanc,
                     char *playerNoir) {
  h->identifiant_partie =
      (long)time(NULL) * 10000 +
      (rand() % 10000); // identifiant unique (statistiquement... )
  h->nbr_coups = 0;
  h->heure_debut = time(NULL);
  strcpy(h->joueur_blanc, playerBlanc);
  strcpy(h->joueur_noir, playerNoir);
}

int historique_enregistrer_coup(move_t coup, historique_partie *h) {
  if (h->nbr_coups >= 512)
    return -1;

  time_t maintenant = time(NULL);

  // Calcul de la durée
  if (h->nbr_coups == 0)
    h->durees[h->nbr_coups] = (int)difftime(maintenant, h->heure_debut);
  else
    h->durees[h->nbr_coups] =
        (int)difftime(maintenant, h->timestamps[h->nbr_coups - 1]);

  h->timestamps[h->nbr_coups] = maintenant;

  h->coups[h->nbr_coups] = coup;
  h->nbr_coups++;
  return 0;
}

void historique_finaliser(historique_partie *h, partie_resultat resultat,
                          int raison) {
  h->resultat = resultat;
  h->raison = raison;
  historique_sauvegarder(h);
  stats_mettre_a_jour(h);
  printf(
      "[historique] Partie %ld finalisée (%d coups, résultat=%d, raison=%d)\n",
      h->identifiant_partie, h->nbr_coups, resultat, raison);
}

int historique_sauvegarder(historique_partie *h) {
  if (h == NULL)
    return -1;

  char filename[128];
  snprintf(filename, sizeof(filename), "data/parties_jouees/history_%ld.txt",
           h->identifiant_partie);

  FILE *f = fopen(filename, "w");
  if (f == NULL) {
    perror("Erreur : ouverture du fichier de sauvegarde");
    return -1;
  }

  // Écriture de l'en-tête
  fprintf(f, "ID,NBR_COUPS,BLANC,NOIR,RESULTAT,RAISON\n");
  fprintf(f, "%ld,%d,%s,%s,%d,%d\n", h->identifiant_partie, h->nbr_coups,
          h->joueur_blanc, h->joueur_noir, h->resultat, h->raison);

  // Écriture de la liste des coups
  char move_text[10];
  for (int i = 0; i < h->nbr_coups; i++) {
    notation_algebrique(h->coups[i], move_text);
    fprintf(f, "%s %d\n", move_text, h->durees[i]);
  }

  fclose(f);

  historique_ajouter_index(h);

  return 0;
}

int historique_charger(historique_partie *h, long id_partie) {
  char filename[128];
  snprintf(filename, sizeof(filename), "data/parties_jouees/history_%ld.txt",
           id_partie);

  FILE *f = fopen(filename, "r");
  if (f == NULL)
    return -1;

  // Lecture en-tête pour récupérer des infos
  char ligne_entete[128];
  fgets(ligne_entete, sizeof(ligne_entete), f); // saute "ID,NBR_COUPS,..."
  fscanf(f, "%ld,%d,%32[^,],%32[^,],%d,%d\n", &h->identifiant_partie,
         &h->nbr_coups, h->joueur_blanc, h->joueur_noir, (int *)&h->resultat,
         &h->raison);

  char move_text[10];
  for (int i = 0; i < h->nbr_coups; i++) {
    int duree = 0;
    if (fscanf(f, "%s %d", move_text, &duree) != 2)
      break;

    h->durees[i] = duree;
    h->coups[i].start.file = move_text[0] - 'a';
    h->coups[i].start.rank = 8 - (move_text[1] - '0');
    h->coups[i].end.file = move_text[2] - 'a';
    h->coups[i].end.rank = 8 - (move_text[3] - '0');
    h->coups[i].promotion = (strlen(move_text) > 4) ? move_text[4] : ' ';
  }

  fclose(f);
  return 0;
}

int historique_exporter_pgn(historique_partie *h, const char *nom_fichier) {
  FILE *f = fopen(nom_fichier, "w");
  if (!f)
    return -1;

  char *resultat_str = "*"; // Valeur par défaut

  if (h->resultat == RESULTAT_PARTIE_WHITE_WIN) {
    resultat_str = "1-0";
  } else if (h->resultat == RESULTAT_PARTIE_NOIR_WIN) {
    resultat_str = "0-1";
  } else if (h->resultat == RESULTAT_PARTIE_DRAW) {
    resultat_str = "1/2-1/2";
  }

  fprintf(f, "[White \"%s\"]\n", h->joueur_blanc);
  fprintf(f, "[Black \"%s\"]\n", h->joueur_noir);
  fprintf(f, "[Result \"%s\"]\n\n", resultat_str);

  char move_str[10];
  for (int i = 0; i < h->nbr_coups; i++) {
    if (i % 2 == 0)
      fprintf(f, "%d. ", (i / 2) + 1);
    notation_algebrique(h->coups[i], move_str);
    fprintf(f, "%s ", move_str);
  }

  fprintf(f, "%s\n", resultat_str);

  fclose(f);
  return 0;
}

stats_joueur stats_lire(const char *pseudo) {
  stats_joueur s;
  memset(&s, 0, sizeof(s));
  strncpy(s.pseudo, pseudo, sizeof(s.pseudo) - 1);

  FILE *f = fopen(STATS_PATH, "r");
  if (!f)
    return s;

  char ligne[128];
  fgets(ligne, sizeof(ligne), f); /* saute la première ligne  */

  while (fgets(ligne, sizeof(ligne), f)) {
    char p[32];
    int v, d, n;
    float r;
    if (sscanf(ligne, "%31[^,],%d,%d,%d,%f", p, &v, &d, &n, &r) == 5) {
      if (strcmp(p, pseudo) == 0) {
        strncpy(s.pseudo, p, sizeof(s.pseudo) - 1);
        s.victoires = v;
        s.defaites = d;
        s.nulles = n;
        s.ratio = r;
        fclose(f);
        return s;
      }
    }
  }

  fclose(f);
  return s; /* joueur non trouvé : struct à zéro */
}

void stats_ecrire(stats_joueur s) {
  /* Charger tous les joueurs existants en mémoire */
  stats_joueur buf[STATS_MAX_JOUEURS];
  int nb = 0;
  int trouve = 0;

  FILE *f = fopen(STATS_PATH, "r");
  if (f) {
    char ligne[128];
    fgets(ligne, sizeof(ligne), f);
    while (nb < STATS_MAX_JOUEURS && fgets(ligne, sizeof(ligne), f)) {
      char p[32];
      int v, d, n;
      float r;
      if (sscanf(ligne, "%31[^,],%d,%d,%d,%f", p, &v, &d, &n, &r) == 5) {
        strncpy(buf[nb].pseudo, p, sizeof(buf[nb].pseudo) - 1);
        buf[nb].victoires = v;
        buf[nb].defaites = d;
        buf[nb].nulles = n;
        buf[nb].ratio = r;
        if (strcmp(p, s.pseudo) == 0) {
          buf[nb] = s; /* remplacer la ligne existante */
          trouve = 1;
        }
        nb++;
      }
    }
    fclose(f);
  }

  if (!trouve)
    buf[nb++] = s; /* nouveau joueur : ajouter en fin */

  /* Réécrire le fichier entier */
  f = fopen(STATS_PATH, "w");
  if (!f) {
    perror("Erreur ouverture stats");
    return;
  }

  fprintf(f, "pseudo,victoires,defaites,nulles,ratio\n");
  for (int i = 0; i < nb; i++)
    fprintf(f, "%s,%d,%d,%d,%.3f\n", buf[i].pseudo, buf[i].victoires,
            buf[i].defaites, buf[i].nulles, buf[i].ratio);

  fclose(f);
}

/* ------------------------------------------------------------------ */
/*  Affichage terminal                                                 */
/* ------------------------------------------------------------------ */

void historique_afficher_stats(const char *pseudo) {
  stats_joueur s = stats_lire(pseudo);

  int total = s.victoires + s.defaites + s.nulles;

  printf("\n╔══════════════════════════════╗\n");
  printf("║  Statistiques : %-12s║\n", pseudo);
  printf("╠══════════════════════════════╣\n");
  printf("║  Parties jouées : %-10d║\n", total);
  printf("║  Victoires      : %-10d║\n", s.victoires);
  printf("║  Défaites       : %-10d║\n", s.defaites);
  printf("║  Nulles         : %-10d║\n", s.nulles);

  if (total > 0)
    printf("║  Ratio          : %-9.1f%%║\n", s.ratio * 100.0f);
  else
    printf("║  Ratio          : N/A       ║\n");

  printf("╚══════════════════════════════╝\n\n");
}

void historique_afficher_parties(const char *pseudo) {
  FILE *f = fopen("data/index_parties.csv", "r");
  if (!f) {
    printf("[historique] Aucune partie enregistrée (index introuvable).\n");
    return;
  }

  /* Correspondance résultat (entier) → label lisible */
  const char *labels_resultat[] = {
      "Blanc gagne", /* RESULTAT_PARTIE_WHITE_WIN = 0 */
      "Noir  gagne", /* RESULTAT_PARTIE_NOIR_WIN  = 1 */
      "Nulle"        /* RESULTAT_PARTIE_DRAW      = 2 */
  };

  printf("\n╔══════════════════════════════════════════════════════╗\n");
  printf("║  Parties de %-39s║\n", pseudo);
  printf("╠══════════╦══════════════╦══════════════╦════════════╣\n");
  printf("║    ID    ║    Blanc     ║    Noir      ║ Résultat   ║\n");
  printf("╠══════════╬══════════════╬══════════════╬════════════╣\n");

  char ligne[256];
  int nb_affichees = 0;

  /* Saute l'en-tête si présente (première ligne ne commençant pas par un
   * chiffre) */
  if (fgets(ligne, sizeof(ligne), f) != NULL) {
    if (ligne[0] >= '0' && ligne[0] <= '9') {
      /* Pas d'en-tête : c'est déjà une donnée, on la traite */
      goto traiter_ligne;
    }
  }

  while (fgets(ligne, sizeof(ligne), f)) {
  traiter_ligne:;
    char tmp[256];
    strncpy(tmp, ligne, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    char *id_str = strtok(tmp, ",");
    char *blanc = strtok(NULL, ",");
    char *noir = strtok(NULL, ",");
    char *res_str = strtok(NULL, ",\n");

    if (!id_str || !blanc || !noir || !res_str)
      continue;

    blanc[strcspn(blanc, "\n")] = '\0';
    noir[strcspn(noir, "\n")] = '\0';
    res_str[strcspn(res_str, "\n")] = '\0';

    /* Filtrer sur le pseudo */
    if (strcmp(pseudo, blanc) != 0 && strcmp(pseudo, noir) != 0)
      continue;

    long id = atol(id_str);
    int res = atoi(res_str);

    const char *res_label = "?";
    if (res >= 0 && res <= 2)
      res_label = labels_resultat[res];

    printf("║ %8ld ║ %-12s ║ %-12s ║ %-10s ║\n", id, blanc, noir, res_label);
    nb_affichees++;
  }

  fclose(f);

  if (nb_affichees == 0)
    printf("║         Aucune partie trouvée pour ce joueur.        ║\n");

  printf("╚══════════╩══════════════╩══════════════╩════════════╝\n");
  printf("  %d partie(s) affichée(s).\n\n", nb_affichees);
}

void stats_mettre_a_jour(historique_partie *h) {
  stats_joueur blanc = stats_lire(h->joueur_blanc);
  stats_joueur noir = stats_lire(h->joueur_noir);

  switch (h->resultat) {
  case RESULTAT_PARTIE_WHITE_WIN:
    blanc.victoires++;
    noir.defaites++;
    break;
  case RESULTAT_PARTIE_NOIR_WIN:
    noir.victoires++;
    blanc.defaites++;
    break;
  case RESULTAT_PARTIE_DRAW:
    blanc.nulles++;
    noir.nulles++;
    break;
  }

  /* Recalcul du ratio pour chaque joueur */
  int total_b = blanc.victoires + blanc.defaites + blanc.nulles;
  int total_n = noir.victoires + noir.defaites + noir.nulles;

  if (total_b > 0) {
    blanc.ratio = (float)blanc.victoires / total_b;
  } else {
    blanc.ratio = 0.0f;
  }

  if (total_n > 0) {
    noir.ratio = (float)noir.victoires / total_n;
  } else {
    noir.ratio = 0.0f;
  }

  stats_ecrire(blanc);
  stats_ecrire(noir);
}

void historique_afficher_profil(const char *pseudo) {
  historique_afficher_stats(pseudo);
  historique_afficher_parties(pseudo);
}
