#ifndef HISTORIQUE_H
#define HISTORIQUE_H

#include "../src/chess.h"   /* move_t, square_t */
#include <time.h>

/* ------------------------------------------------------------------ */
/*  Résultats et raisons de fin de partie                             */
/* ------------------------------------------------------------------ */

#define RESULTAT_PARTIE_WHITE_WIN  1
#define RESULTAT_PARTIE_NOIR_WIN   2
#define RESULTAT_PARTIE_DRAW       3

#define RAISON_CHECKMATE   1
#define RAISON_STALEMATE   2
#define RAISON_DRAW        3
#define RAISON_RESIGN      4   /* abandon */
#define RAISON_DISCONNECT  5   /* déconnexion réseau */

/* ------------------------------------------------------------------ */
/*  Structures                                                         */
/* ------------------------------------------------------------------ */

/* move_t est défini dans chess.h ; on lui ajoute ici les champs
 * nécessaires à l'historique via une structure étendue si besoin,
 * mais pour rester compatible on étend directement move_t dans chess.h.
 * Les champs suivants DOIVENT être présents dans move_t :
 *   time_t  timestamp;
 *   int     duree_secondes;
 * (ajoutez-les dans chess.h si ce n'est pas encore le cas)
 */

typedef struct historique_partie {
    long   identifiant_partie;   /* = numéro de ligne dans l'index CSV */
    int    nbr_coups;
    char   joueur_blanc[33];
    char   joueur_noir[33];
    int    resultat;             /* RESULTAT_PARTIE_* */
    int    raison;               /* RAISON_* */
    time_t heure_debut;
    move_t coups[512];
} historique_partie;

typedef struct stats_joueur {
    char  pseudo[33];
    int   victoires;
    int   defaites;
    int   nulles;
    float ratio;
} stats_joueur;

/* ------------------------------------------------------------------ */
/*  API                                                                */
/* ------------------------------------------------------------------ */

/*
 * Initialisation
 * --------------
 * Appeler depuis room.c dès que la partie démarre.
 * L'ID n'est PAS encore attribué ici ; il le sera à la sauvegarde.
 */
void historique_init_partie(historique_partie *h,
                             const char *pseudo_blanc,
                             const char *pseudo_noir);

/* Alias rétro-compatible */
void historique_init(historique_partie *h,
                     char *playerBlanc,
                     char *playerNoir);

/*
 * Enregistrement d'un coup
 * -------------------------
 * Appeler depuis game.c / room.c après chaque chess_apply_move().
 * Retourne 0 si OK, -1 si la limite de 512 coups est atteinte.
 */
int historique_enregistrer_coup(move_t coup, historique_partie *h);

/*
 * Fin de partie
 * -------------
 * Appeler depuis room.c (mat, pat, abandon, nulle…).
 * Renseigne résultat + raison, puis sauvegarde tout (fichier + index + stats).
 *
 * Exemple :
 *   historique_finaliser(&room->historique,
 *                         RESULTAT_PARTIE_WHITE_WIN,
 *                         RAISON_CHECKMATE);
 */
int historique_finaliser(historique_partie *h, int resultat, int raison);

/*
 * Sauvegarde manuelle (si on préfère gérer soi-même résultat/raison)
 * ------------------------------------------------------------------
 * Attribue l'ID, écrit le fichier, met à jour l'index et les stats.
 */
int historique_sauvegarder(historique_partie *h);

/*
 * Lecture
 */
int   historique_charger(historique_partie *h, long id_partie);
long *historique_lister_parties_joueur(const char *pseudo, int *nb_parties);
int   historique_exporter_pgn(historique_partie *h, const char *nom_fichier);

/*
 * Statistiques
 */
stats_joueur stats_lire(const char *pseudo);
void         stats_ecrire(stats_joueur s);
void         stats_mettre_a_jour(historique_partie *h);

#endif /* HISTORIQUE_H */