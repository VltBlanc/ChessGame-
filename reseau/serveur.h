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

// Traite une requête reçue du client selon son type (MsgHeader).
// Envoie la réponse appropriée sur dSC.
void traiter_requete(int dSC, MsgHeader *header, void *payload);

#endif // SERVEUR_H
