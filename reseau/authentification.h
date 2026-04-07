#ifndef AUTHENTIFICATION_H
#define AUTHENTIFICATION_H


//fonction :  créer_compte, connexion_compte, 
//Ne pas oublier de relier ces fonctions avec les requêtes dans protocol.c 




int connexion( char *username, char *password);
int create_account( char *username, char *password);

#endif