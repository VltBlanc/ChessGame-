#ifndef NETWORK_H
#define NETWORK_H

#include "../src/chess.h"
#include <stdint.h>



typedef enum {
    // Authentification
    REQ_AUTH            = 1,  // Client --> Serveur : demande authentification
    REP_AUTH            = 2,  // Serveur --> Client : réponse authentification
    
    // Matchmaking
    REQ_SEARCH          = 3,  // Client --> Serveur : chercher partie
    REP_SEARCH          = 4,  // Serveur --> Client : réponse recherche
    
    // Partie 
    REQ_MOVE            = 5,  // Client --> Serveur : coup joué 
    REP_MOVE            = 6,  // Serveur --> Client : nouvel état du plateau / coup adverse
   
    REP_RESULTAT_PARTIE = 7,  // Serveur --> Client : résultat final de la partie
    REP_MOVE_ACK        = 8,  // Serveur --> Client : coup accepté ou refusé

    // connexion
    REQ_CONNEXION       = 9,  // Client --> Serveur : demande connexion
    REP_CONNEXION       = 10, // Serveur --> Client : réponse connexion
    
    // Quitter partie
    REQ_RESIGN          = 11, // Client --> Serveur : quitter la room

    // Fin de partie (mat, pat, nulle)
    REQ_GAME_OVER       = 12, // Client --> Serveur : la partie est terminée

    //historique
    REQ_HISTORIQUE      = 13, // Client --> Serveur : demande historique
    REP_HISTORIQUE      = 14, // Serveur --> Client : réponse historique

    //historique
    REQ_HISTORIQUE_PARTIE      = 15, // Client --> Serveur : demande historique
    REP_HISTORIQUE_PARTIE      = 16, // Serveur --> Client : réponse historique
} MsgType;

typedef enum {
    STATUS_OK       = 0,
    STATUS_ERR_AUTH = 1,   // Erreur username ou password
    STATUS_ERR_MOVE = 2,   // Coup illégal
} StatusCode;

typedef enum {
    GAME_CHECKMATE    = 1,  // Échec et mat
    GAME_STALEMATE    = 2,  // Pat
    GAME_DRAW_AGREE   = 3,  // Accord mutuel
    GAME_DRAW_50      = 4,  // Règle des 50 coups
    GAME_DRAW_REPEAT  = 5,  // Répétition 3x
    GAME_RESIGN       = 6,  // Abandon
} GameOverPartie;


// HEADER : Fondamental ! il doit être rempli pour chaque requête envoyée 
// et lu à chaque réception pour "savoir" quel type de requête on a reçu 
typedef struct {
    int      type;       // MsgType
    uint32_t length;     // taille du payload en octets
} MsgHeader;

// REQ_AUTH
typedef struct {
    char password[32];
    char username[32];
} str_RequAuth;

// REP_AUTH
typedef struct {
    int  resultat;       // StatusCode — STATUS_OK ou STATUS_ERR_AUTH
    char username[32];
} str_RepAuth;

// REQ_MOVE
typedef struct {
    int dep_col;         // colonne départ  [0;7]
    int dep_ligne;       // ligne départ    [0;7]
    int vers_col;        // colonne arrivée [0;7]
    int vers_ligne;      // ligne arrivée   [0;7]
} str_ReqMove;

// REP_MOVE 
typedef struct {
    int resultat;        // StatusCode — STATUS_OK ou STATUS_ERR_MOVE
    int dep_col;
    int dep_ligne;
    int vers_col;
    int vers_ligne;
    int trait;           // 0 = trait au noir, 1 = trait au blanc
    int en_echec;        // 1 si le joueur au trait est en échec
} str_RepMove;

// REP_RESULTAT_PARTIE
typedef struct {
    int  raison;         // GameOverPartie
    int  gagnant;        // 0 = noirs, 1 = blancs, 2 = nul
    char nom_gagnant[32];
} str_RepResultatPartie;

// REQ_GAME_OVER
typedef struct {
    int  raison;         // GameOverPartie (GAME_CHECKMATE, GAME_STALEMATE, etc.)
    int  gagnant;        // 0 = noirs, 1 = blancs, 2 = nul
} str_ReqGameOver;

// REP_SEARCH 
typedef struct {
    int  resultat;       // STATUS_OK = match trouvé, sinon on a  STATUS_ERR = en attente
    char adversaire[32]; // nom de l'adversaire si match trouvé
    int  couleur;        // 0 = noirs, 1 = blancs
} str_RepSearch;

// rep_historique
typedef struct {
    int resultat;        // StatusCode — STATUS_OK ou STATUS_ERR_MOVE
    char historique[10000];
    char username[32];
} str_RepHistorique;

//req_historique
typedef struct {
    long id_partie;
} str_RequHistorique;

// (Les structures redondantes en bas de fichier ont été supprimées car elles causaient des conflits et on préfère passer par les structs plus complètes définies plus haut)

int send_message(int descripteur, MsgType type, void *payload, uint32_t size);
int recv_message(int descripteur, MsgHeader *header, void *buf, uint32_t buf_size);

#endif // PROTOCOL_H


