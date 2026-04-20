#ifndef SERVEUR_H
#define SERVEUR_H

#include "protocol.h"
#include "room.h"

// Prototypes des fonctions
int traiter_requete_debut(int dSC, MsgHeader *header, void *payload);
int traiter_requete_jeu(int dSC, MsgHeader *header, void *payload, const char* pseudo);
int lancement_serveur(int argc, char *argv[]);

#endif