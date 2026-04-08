#ifndef CLIENT_H
#define CLIENT_H

// pour lancer :
// ./client IP_serveur numero_du_port
// si serveur sur le meme pc IP=127.0.0.1

// Lance le client : connexion au serveur, envoi de commandes et réception des réponses.
// Retourne 0 si tout s'est bien passé.
int lancement_client(int argc, char *argv[]);

// Traite une requête reçue du client selon son type (MsgHeader).
// Envoie la réponse appropriée sur dSC.
void traiter_requete(int dS, char *m);

#endif // CLIENT_H
