#ifndef HISTORIQUE_H
#define HISTORIQUE_H

#include "../src/chess.h"

/* Résultats possibles d'une partie */
typedef enum {
  RESULTAT_PARTIE_WHITE_WIN,
  RESULTAT_PARTIE_NOIR_WIN,
  RESULTAT_PARTIE_DRAW
} partie_resultat;

/* Structure principale pour stocker l'historique d'une partie */
typedef struct {
  long identifiant_partie;  /* ID unique basé sur le temps (timestamp) */
  move_t coups[512];        /* Liste des coups joués */
  time_t timestamps[512];   /* Horodatage de chaque coup */
  int durees[512];          /* Durée en secondes de chaque coup */
  int nbr_coups;            /* Nombre de coups enregistrés */
  char joueur_blanc[32];    /* Nom du joueur blanc */
  char joueur_noir[32];     /* Nom du joueur noir */
  partie_resultat resultat; /* Gagnant ou match nul */
  int raison;               /* Cause de la fin (mat, abandon, etc.) */
  time_t heure_debut;
} historique_partie;

/* Statistiques d'un joueur */
typedef struct {
  char pseudo[32];
  int victoires;
  int defaites;
  int nulles;
  float ratio; /* victoires / total parties jouées */
} stats_joueur;

/* Lit les stats d'un joueur depuis un CSV  */
stats_joueur stats_lire(const char *pseudo);

/* Met à jour le CSV pour un joueur  */
void stats_ecrire(stats_joueur s);

/* Met à jour les stats des deux joueurs après une partie */
void stats_mettre_a_jour(historique_partie *h);

/* Convertit un coup move_t en texte (ex: e2e4) */
void notation_algebrique(move_t move, char *buffer);

/* Initialise l'historique avec les noms des joueurs et génère l'ID */
void historique_init(historique_partie *h, char *playerBlanc, char *playerNoir);

/* Ajoute un nouveau coup dans le tableau de l'historique en mémoire */
int historique_enregistrer_coup(move_t coup, historique_partie *h);

/* Finalise l'historique : enregistre résultat, sauvegarde et met à jour les
 * stats */
void historique_finaliser(historique_partie *h, partie_resultat resultat,
                          int raison);

/* Sauvegarde la partie (détails + index centralisé) */
int historique_sauvegarder(historique_partie *h);

/* Charge une partie complète depuis son fichier .txt via son ID */
int historique_charger(historique_partie *h, long id_partie);

/* Ajoute une entrée résumée dans le fichier index_parties.csv */
void historique_ajouter_index(historique_partie *h);

/* Récupère la liste des IDs de parties pour un pseudo donné depuis l'index */
long *historique_lister_parties_joueur(const char *pseudo, int *nb_parties);

/* Exporte la partie au format PGN pour les logiciels d'échecs tiers */
int historique_exporter_pgn(historique_partie *h, const char *nom_fichier);

/*
 * Affiche dans le terminal les statistiques d'un joueur (victoires,
 * défaites, nulles, ratio). Lit directement dans le CSV.
 * Exemple d'appel : historique_afficher_stats("alice");
 */
void historique_afficher_stats(const char *pseudo);

/*
 * Affiche dans le terminal la liste de tous les IDs de parties
 * auxquelles le joueur a participé, avec blanc/noir et résultat.
 * Exemple d'appel : historique_afficher_parties("alice");
 */
void historique_afficher_parties(const char *pseudo);

#endif