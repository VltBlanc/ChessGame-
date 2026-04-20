#ifndef AUTHENTIFICATION_H
#define AUTHENTIFICATION_H


// Hash le mot de passe 
void hash_password(const char *password, char *output_hex);

int logique_create_account(const char *username, const char *password);
int logique_connexion(const char *username, const char *password);

#endif