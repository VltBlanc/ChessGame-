#ifndef NETWORK_H
#define NETWORK_H

#include "chess.h"

// Initialise la connexion
int network_connect(const char *ip, int port);
// Envoie un coup move_t
int network_send_move(int socket, move_t move);
// Attend et reçoit un coup move_t du serveur
int network_receive_move(int socket, move_t *move);

    // Partie 
    REQ_MOVE            = 5,  // Client --> Serveur : coup joué 
    REP_MOVE            = 6,  // Serveur --> Client : nouvel état du plateau
   
    REP_RESULTAT_PARTIE = 7,  // Serveur --> Client : résultat final de la partie
    REP_MOVE            = 8,  // Serveur --> Client : coup accepté ou refusé


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
    int plateau[64];     // état du plateau
    int trait;           // 0 = trait au noir, 1 = trait au blanc
    int en_echec;        // 1 si le joueur au trait est en échec
} str_RepMove;

// REP_RESULTAT_PARTIE
typedef struct {
    int  raison;         // GameOverPartie
    int  gagnant;        // 0 = noirs, 1 = blancs, 2 = nul
    char nom_gagnant[32];
} str_RepResultatPartie;

// REP_SEARCH 
typedef struct {
    int  resultat;       // STATUS_OK = match trouvé, sinon on a  STATUS_ERR = en attente
    char adversaire[32]; // nom de l'adversaire si match trouvé
    int  couleur;        // 0 = noirs, 1 = blancs
} str_RepSearch;



// HEADER : Fondamental ! il doit être rempli pour chaque requête envoyée 
// et lu à chaque réception pour "savoir" quel type de requête on a reçu 
typedef struct {
    int      type;       // MsgType
    uint32_t length;     // taille du payload en octets
} MsgHeader;

// REP_RESULTAT_PARTIE
typedef struct{
    char username[32];
    int resultat; // resultat = 0 si l'utilisateur à gagner et resultat = 1 si l'utilisateur à perdu
} str_RepResultatPartie;

// REP_MOVE_ACCEPT
typedef struct{
    char username[32];
    int RepMOVE ; // RepMove = 0 si le movement est accepter et RepMove si le mouvement est refuser
} str_RepMove;


int send_message(int descripteur, MsgType type, void *payload, uint32_t size);
int recv_message(int descripteur, MsgHeader *header, void *buf, uint32_t buf_size);

#endif // PROTOCOL_H

