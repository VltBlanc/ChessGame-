#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "historique.h"

#define INDEX_PATH        "../data/index_parties.csv"
#define PARTIES_DIR       "../data/parties_jouees"
#define STATS_PATH        "../data/stats_joueurs.csv"
#define STATS_MAX_JOUEURS 4096

/* ------------------------------------------------------------------ */
/*  Utilitaires internes                                               */
/* ------------------------------------------------------------------ */

/* Retourne le nombre de lignes de données dans l'index (sans l'en-tête).
 * C'est ce nombre qui sera utilisé comme prochain ID. */
static long index_compter_lignes(void) {
    FILE *f = fopen(INDEX_PATH, "r");
    if (!f) return 0;

    long count = 0;
    char ligne[256];
    /* Saute l'en-tête si présente */
    if (fgets(ligne, sizeof(ligne), f) == NULL) { fclose(f); return 0; }
    /* Si la première ligne ne commence pas par un chiffre c'est l'en-tête,
     * sinon c'est déjà une donnée — on la compte. */
    if (ligne[0] >= '0' && ligne[0] <= '9') count = 1;

    while (fgets(ligne, sizeof(ligne), f)) count++;

    fclose(f);
    return count;
}

static void notation_algebrique(move_t move, char *buffer) {
    char start_col = 'a' + move.start.file;
    char end_col   = 'a' + move.end.file;
    int  start_row = 8 - move.start.rank;
    int  end_row   = 8 - move.end.rank;

    if (move.promotion != ' ' && move.promotion != '\0') {
        sprintf(buffer, "%c%d%c%d%c",
                start_col, start_row, end_col, end_row, move.promotion);
    } else {
        sprintf(buffer, "%c%d%c%d",
                start_col, start_row, end_col, end_row);
    }
}

/* Ajoute une ligne dans l'index CSV.
 * L'ID doit déjà être renseigné dans h->identifiant_partie. */
static void historique_ajouter_index(historique_partie *h) {
    /* Crée l'en-tête si le fichier n'existe pas encore */
    FILE *test = fopen(INDEX_PATH, "r");
    int besoin_entete = (test == NULL);
    if (test) fclose(test);

    FILE *f = fopen(INDEX_PATH, "a");
    if (!f) { perror("[historique] Erreur ouverture index"); return; }

    if (besoin_entete)
        fprintf(f, "id,blanc,noir,resultat\n");

    fprintf(f, "%ld,%s,%s,%d\n",
            h->identifiant_partie,
            h->joueur_blanc,
            h->joueur_noir,
            h->resultat);

    fclose(f);
}

/* ------------------------------------------------------------------ */
/*  API publique                                                       */
/* ------------------------------------------------------------------ */

/*
 * historique_init_partie()
 * -------------------------
 * À appeler depuis room.c quand la room passe en état EN_JEU.
 * Initialise la structure mais n'attribue PAS encore l'ID
 * (celui-ci sera calculé à la sauvegarde).
 *
 * Exemple d'appel dans room.c :
 *   historique_init_partie(&room->historique,
 *                           room->pseudo_blanc,
 *                           room->pseudo_noir);
 */
void historique_init_partie(historique_partie *h,
                             const char *pseudo_blanc,
                             const char *pseudo_noir) {
    memset(h, 0, sizeof(*h));
    h->identifiant_partie = -1;          /* non encore attribué */
    h->nbr_coups          = 0;
    h->heure_debut        = time(NULL);
    strncpy(h->joueur_blanc, pseudo_blanc, sizeof(h->joueur_blanc) - 1);
    strncpy(h->joueur_noir,  pseudo_noir,  sizeof(h->joueur_noir)  - 1);
}

/* Alias rétro-compatible (ancienne signature) */
void historique_init(historique_partie *h,
                     char *playerBlanc,
                     char *playerNoir) {
    historique_init_partie(h, playerBlanc, playerNoir);
}

/*
 * historique_enregistrer_coup()
 * ------------------------------
 * À appeler depuis game.c / room.c à chaque coup validé.
 * Compatible avec le reste du code existant — signature inchangée.
 *
 * Exemple d'appel dans game.c (après chess_apply_move) :
 *   historique_enregistrer_coup(*move, &room->historique);
 */
int historique_enregistrer_coup(move_t coup, historique_partie *h) {
    if (h->nbr_coups >= 512) return -1;

    time_t maintenant = time(NULL);

    coup.duree_secondes =
        (h->nbr_coups == 0)
        ? (int)difftime(maintenant, h->heure_debut)
        : (int)difftime(maintenant, h->coups[h->nbr_coups - 1].timestamp);

    coup.timestamp = maintenant;

    h->coups[h->nbr_coups] = coup;
    h->nbr_coups++;
    return 0;
}

/*
 * historique_finaliser()
 * -----------------------
 * À appeler depuis room.c (fin de partie : mat, pat, abandon, nulle).
 * Renseigne le résultat + la raison, puis sauvegarde tout.
 *
 * Exemple d'appel dans room.c :
 *   historique_finaliser(&room->historique,
 *                         RESULTAT_PARTIE_WHITE_WIN,
 *                         RAISON_CHECKMATE);
 */
int historique_finaliser(historique_partie *h, int resultat, int raison) {
    h->resultat = resultat;
    h->raison   = raison;
    return historique_sauvegarder(h);
}

/*
 * historique_sauvegarder()
 * -------------------------
 * Attribue l'ID (= numéro de la prochaine ligne de l'index),
 * écrit le fichier de la partie, puis met à jour l'index et les stats.
 */
int historique_sauvegarder(historique_partie *h) {
    if (!h) return -1;

    /* Attribution de l'ID si pas encore fait */
    if (h->identifiant_partie < 0)
        h->identifiant_partie = index_compter_lignes();

    /* Écriture du fichier de partie */
    char filename[256];
    snprintf(filename, sizeof(filename),
             "%s/history_%ld.txt", PARTIES_DIR, h->identifiant_partie);

    FILE *f = fopen(filename, "w");
    if (!f) { perror("[historique] Erreur ouverture fichier partie"); return -1; }

    fprintf(f, "ID,NBR_COUPS,BLANC,NOIR,RESULTAT,RAISON\n");
    fprintf(f, "%ld,%d,%s,%s,%d,%d\n",
            h->identifiant_partie, h->nbr_coups,
            h->joueur_blanc, h->joueur_noir,
            h->resultat, h->raison);

    char move_text[10];
    for (int i = 0; i < h->nbr_coups; i++) {
        notation_algebrique(h->coups[i], move_text);
        fprintf(f, "%s %d\n", move_text, h->coups[i].duree_secondes);
    }

    fclose(f);

    /* Mise à jour de l'index et des stats */
    historique_ajouter_index(h);
    stats_mettre_a_jour(h);

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Lecture / export                                                   */
/* ------------------------------------------------------------------ */

int historique_charger(historique_partie *h, long id_partie) {
    char filename[256];
    snprintf(filename, sizeof(filename),
             "%s/history_%ld.txt", PARTIES_DIR, id_partie);

    FILE *f = fopen(filename, "r");
    if (!f) return -1;

    char ligne_entete[128];
    fgets(ligne_entete, sizeof(ligne_entete), f); /* saute l'en-tête */

    fscanf(f, "%ld,%d,%32[^,],%32[^,],%d,%d\n",
           &h->identifiant_partie,
           &h->nbr_coups,
           h->joueur_blanc,
           h->joueur_noir,
           (int *)&h->resultat,
           &h->raison);

    char move_text[10];
    for (int i = 0; i < h->nbr_coups; i++) {
        int duree = 0;
        if (fscanf(f, "%s %d", move_text, &duree) != 2) break;

        h->coups[i].duree_secondes = duree;
        h->coups[i].start.file = move_text[0] - 'a';
        h->coups[i].start.rank = 8 - (move_text[1] - '0');
        h->coups[i].end.file   = move_text[2] - 'a';
        h->coups[i].end.rank   = 8 - (move_text[3] - '0');
        h->coups[i].promotion  =
            (strlen(move_text) > 4) ? move_text[4] : ' ';
    }

    fclose(f);
    return 0;
}

long *historique_lister_parties_joueur(const char *pseudo, int *nb_parties) {
    FILE *f = fopen(INDEX_PATH, "r");
    if (!f) { *nb_parties = 0; return NULL; }

    char ligne[256];
    /* Saute l'en-tête */
    fgets(ligne, sizeof(ligne), f);

    int compteur = 0;
    while (fgets(ligne, sizeof(ligne), f)) {
        char *id_str = strtok(ligne, ",");
        char *blanc  = strtok(NULL, ",");
        char *noir   = strtok(NULL, ",");
        if (id_str && blanc && noir) {
            blanc[strcspn(blanc, "\n")] = '\0';
            noir [strcspn(noir,  "\n")] = '\0';
            if (strcmp(pseudo, blanc) == 0 || strcmp(pseudo, noir) == 0)
                compteur++;
        }
    }

    long *ids = malloc(compteur * sizeof(long));
    if (!ids) { fclose(f); *nb_parties = 0; return NULL; }

    rewind(f);
    fgets(ligne, sizeof(ligne), f); /* saute l'en-tête à nouveau */

    int i = 0;
    while (fgets(ligne, sizeof(ligne), f)) {
        char *id_str = strtok(ligne, ",");
        char *blanc  = strtok(NULL, ",");
        char *noir   = strtok(NULL, ",");
        if (id_str && blanc && noir) {
            blanc[strcspn(blanc, "\n")] = '\0';
            noir [strcspn(noir,  "\n")] = '\0';
            if (strcmp(pseudo, blanc) == 0 || strcmp(pseudo, noir) == 0)
                ids[i++] = atol(id_str);
        }
    }

    fclose(f);
    *nb_parties = compteur;
    return ids;
}

int historique_exporter_pgn(historique_partie *h, const char *nom_fichier) {
    FILE *f = fopen(nom_fichier, "w");
    if (!f) return -1;

    const char *resultat_str = "*";
    if      (h->resultat == RESULTAT_PARTIE_WHITE_WIN) resultat_str = "1-0";
    else if (h->resultat == RESULTAT_PARTIE_NOIR_WIN)  resultat_str = "0-1";
    else if (h->resultat == RESULTAT_PARTIE_DRAW)      resultat_str = "1/2-1/2";

    fprintf(f, "[White \"%s\"]\n", h->joueur_blanc);
    fprintf(f, "[Black \"%s\"]\n", h->joueur_noir);
    fprintf(f, "[Result \"%s\"]\n\n", resultat_str);

    char move_str[10];
    for (int i = 0; i < h->nbr_coups; i++) {
        if (i % 2 == 0) fprintf(f, "%d. ", (i / 2) + 1);
        notation_algebrique(h->coups[i], move_str);
        fprintf(f, "%s ", move_str);
    }

    fprintf(f, "%s\n", resultat_str);
    fclose(f);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Statistiques                                                       */
/* ------------------------------------------------------------------ */

stats_joueur stats_lire(const char *pseudo) {
    stats_joueur s;
    memset(&s, 0, sizeof(s));
    strncpy(s.pseudo, pseudo, sizeof(s.pseudo) - 1);

    FILE *f = fopen(STATS_PATH, "r");
    if (!f) return s;

    char ligne[128];
    fgets(ligne, sizeof(ligne), f); /* en-tête */

    while (fgets(ligne, sizeof(ligne), f)) {
        char p[32]; int v, d, n; float r;
        if (sscanf(ligne, "%31[^,],%d,%d,%d,%f", p, &v, &d, &n, &r) == 5) {
            if (strcmp(p, pseudo) == 0) {
                strncpy(s.pseudo, p, sizeof(s.pseudo) - 1);
                s.victoires = v;
                s.defaites  = d;
                s.nulles    = n;
                s.ratio     = r;
                fclose(f);
                return s;
            }
        }
    }

    fclose(f);
    return s;
}

void stats_ecrire(stats_joueur s) {
    stats_joueur buf[STATS_MAX_JOUEURS];
    int nb = 0, trouve = 0;

    FILE *f = fopen(STATS_PATH, "r");
    if (f) {
        char ligne[128];
        fgets(ligne, sizeof(ligne), f); /* en-tête */
        while (nb < STATS_MAX_JOUEURS && fgets(ligne, sizeof(ligne), f)) {
            char p[32]; int v, d, n; float r;
            if (sscanf(ligne, "%31[^,],%d,%d,%d,%f", p, &v, &d, &n, &r) == 5) {
                strncpy(buf[nb].pseudo, p, sizeof(buf[nb].pseudo) - 1);
                buf[nb].victoires = v;
                buf[nb].defaites  = d;
                buf[nb].nulles    = n;
                buf[nb].ratio     = r;
                if (strcmp(p, s.pseudo) == 0) {
                    buf[nb] = s;
                    trouve  = 1;
                }
                nb++;
            }
        }
        fclose(f);
    }

    if (!trouve) buf[nb++] = s;

    f = fopen(STATS_PATH, "w");
    if (!f) { perror("[historique] Erreur ouverture stats"); return; }

    fprintf(f, "pseudo,victoires,defaites,nulles,ratio\n");
    for (int i = 0; i < nb; i++)
        fprintf(f, "%s,%d,%d,%d,%.3f\n",
                buf[i].pseudo, buf[i].victoires,
                buf[i].defaites, buf[i].nulles, buf[i].ratio);

    fclose(f);
}

void stats_mettre_a_jour(historique_partie *h) {
    stats_joueur blanc = stats_lire(h->joueur_blanc);
    stats_joueur noir  = stats_lire(h->joueur_noir);

    switch (h->resultat) {
        case RESULTAT_PARTIE_WHITE_WIN:
            blanc.victoires++; noir.defaites++;  break;
        case RESULTAT_PARTIE_NOIR_WIN:
            noir.victoires++;  blanc.defaites++; break;
        case RESULTAT_PARTIE_DRAW:
            blanc.nulles++; noir.nulles++;       break;
    }

    int total_b = blanc.victoires + blanc.defaites + blanc.nulles;
    int total_n = noir.victoires  + noir.defaites  + noir.nulles;

    blanc.ratio = total_b > 0 ? (float)blanc.victoires / total_b : 0.0f;
    noir.ratio  = total_n > 0 ? (float)noir.victoires  / total_n : 0.0f;

    stats_ecrire(blanc);
    stats_ecrire(noir);
}