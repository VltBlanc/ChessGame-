#ifndef AUTHENTIFICATION_H
#define AUTHENTIFICATION_H

// Crée un compte (écrit dans data_Joueur.csv).
// Retourne 1 si succès, 0 si le pseudo existe déjà ou entrée invalide.
int logique_create_account(char *username, char *password);

// Vérifie les identifiants (lecture data_Joueur.csv).
// Retourne 1 si correspondance trouvée, 0 sinon.
int logique_connexion(char *username, char *password);

#endif