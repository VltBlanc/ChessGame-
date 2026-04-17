#ifndef CLIENT_H
#define CLIENT_H

// pour lancer :
// ./client IP_serveur numero_du_port
// si serveur sur le meme pc IP=127.0.0.1

// Lance le client : connexion au serveur, envoi de commandes et réception des réponses.
// Retourne 0 si tout s'est bien passé.
int lancement_client(int argc, char *argv[]);

// Traite les requêtes d'authentification et de création de compte.
// Retourne STATUS_OK (0) ou STATUS_ERR_AUTH (-1).
int traiter_requete_debut(int dS, MsgHeader *header, void *payload);

// Traite les requêtes de jeu (ex: recherche de partie).
int traiter_requete_jeu(int dS, MsgHeader *header, void *payload);

// Boucle principale de jeu côté client
void client_boucle_jeu(int dS, int ma_couleur);

#endif // CLIENT_H
