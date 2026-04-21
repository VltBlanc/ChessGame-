#ifndef HISTORIQUE_H
#define HISTORIQUE_H

#include "../src/chess.h"
#include <time.h>
typedef enum {
  RESULTAT_PARTIE_WHITE_WIN,
  RESULTAT_PARTIE_NOIR_WIN,
  RESULTAT_PARTIE_DRAW
} partie_resultat;

/* Données complètes d'une partie en mémoire */
typedef struct {
  long identifiant_partie; /* ID unique généré à l'initialisation */
  move_t coups[512];       /* Coups joués (max 512) */
  time_t timestamps[512];  /* Horodatage de chaque coup */
  int durees[512];         /* Durée en secondes par coup */
  int nbr_coups;           /* Nombre de coups enregistrés */
  char joueur_blanc[32];
  char joueur_noir[32];
  partie_resultat resultat;
  int raison; /* Cause de fin (mat, abandon, etc.) */
  time_t heure_debut;
} historique_partie;

/* Statistiques cumulées d'un joueur (ratio = victoires / total) */
typedef struct {
  char pseudo[32];
  int victoires;
  int defaites;
  int nulles;
  float ratio;
} stats_joueur;

/*  CYCLE DE VIE  —  init → enregistrer_coup (×N) → finaliser  */

/* Initialise une nouvelle partie (ID, pseudos, heure de début) */
void historique_init(historique_partie *h, char *playerBlanc, char *playerNoir);

/* Ajoute un coup en mémoire. Retourne -1 si limite 512 atteinte */
int historique_enregistrer_coup(move_t coup, historique_partie *h);

/* Clôture la partie : sauvegarde + mise à jour des stats */
void historique_finaliser(historique_partie *h, partie_resultat resultat,
                          int raison);

/* Persistance */

/* Sauvegarde dans data/parties_jouees/history_<ID>.txt. Retourne 0 ou -1 */
int historique_sauvegarder(historique_partie *h);

/* Charge une partie par son ID. Retourne 0 ou -1 si introuvable */
int historique_charger(historique_partie *h, long id_partie);

/* Ajoute une ligne résumée dans data/index_parties.csv */
void historique_ajouter_index(historique_partie *h);

/* Consultation */

/* Retourne les IDs de parties d'un joueur (tableau à free() par l'appelant) */
long *historique_lister_parties_joueur(const char *pseudo, int *nb_parties);

/* Retourne la partie au format PGN (chaîne à free() par l'appelant) */
char *historique_exporter_pgn(long id);

/* Convertit un move_t en texte (ex: "e2e4"). buffer >= 6 octets */
void notation_algebrique(move_t move, char *buffer);

/* STATISTIQUES */

/* Lit les stats depuis le CSV. Retourne une struct à zéro si inconnu */
stats_joueur stats_lire(const char *pseudo);

/* Met à jour (ou crée) la ligne du joueur dans le CSV */
void stats_ecrire(stats_joueur s);

/* Met à jour les stats des deux joueurs (appelé par historique_finaliser) */
void stats_mettre_a_jour(historique_partie *h);

/* AFFICHAGE  —  toutes les chaînes retournées sont à free() par l'appelant */

/* Stats du joueur (parties, victoires, défaites, nulles, ratio) */
char *historique_afficher_stats(const char *pseudo);

/* Liste des parties du joueur (ID, blanc, noir, résultat) */
char *historique_afficher_parties(const char *pseudo);

/* Combine afficher_stats + afficher_parties */
char *historique_afficher_profil(const char *pseudo);

#endif