#ifndef SERVEUR_H
#define SERVEUR_H

#include "protocol.h"
#include "room.h"
#include "authentification.h"


// pour lancer :
// ./serveur numero_du_port

// Lance le serveur : écoute sur le port donné et gère chaque client dans un thread dédié.
// Retourne 0 si tout s'est bien passé.
int lancement_serveur(int argc, char *argv[]);

// Fonction exécutée par chaque thread client (prend un int* vers le descripteur socket).
void* routine_client(void* arg);

// Traite les requêtes d'authentification et de création de compte.
// Retourne STATUS_OK (0) ou STATUS_ERR_AUTH (-1).
int traiter_requete_debut(int dSC, MsgHeader *header, void *payload);

// Traite les requêtes de jeu (ex: recherche de partie).
int traiter_requete_jeu(int dSC, MsgHeader *header, void *payload, const char *username);

#endif // SERVEUR_H
