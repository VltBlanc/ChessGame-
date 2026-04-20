#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sha256.h"

 
void hash_password(const char *password, char *output_hex) {
        BYTE hash[32];
        SHA256_CTX ctx;
 
        sha256_init(&ctx);
        sha256_update(&ctx, (BYTE*)password, strlen(password));
        sha256_final(&ctx, hash);
 
        for (int i = 0; i < 32; i++)
            sprintf(output_hex + (i * 2), "%02x", hash[i]);
        output_hex[64] = '\0';
}

int logique_create_account( char *username, char *password) {

    char password_hash[65];
    hash_password(password, password_hash);

    //Précondition : Taille des entrées 
    if (strlen(username) >= 32 || strlen(password) >= 32) {
        printf("Erreur : Pseudo ou mot de passe trop long.\n");
        return 0;
    }

    char ligne[1024];
    int existe_deja = 0;


    FILE *op = fopen("data/data_Joueur.csv", "r");
    if (op != NULL) {
        while(fgets(ligne, sizeof(ligne),op) ){ //Condition d'arrêt : curseur arrive sur une ligne vide 
            
            ligne[strcspn(ligne, "\r\n")] = 0; //permet de nettoyer chaque fin de ligne car pris en compte dans les strcmp
            char *pseudo_dans_csv = strtok(ligne, ",");
            if( username != NULL && strcmp(username, pseudo_dans_csv) == 0) {    
                existe_deja = 1;
                break;
            }
        }
        fclose(op);
    }

    if (existe_deja == 0) {
        FILE *op = fopen("data/data_Joueur.csv", "a");
        if (op == NULL) {
            perror("Erreur ouverture fichier CSV en écriture");
            return 0;
        }
        fprintf(op, "%s,%s\n", username, password_hash);
        fclose(op);
        printf("Le compte est crée ! \n ");
        return 1; // Écriture réussie
    }
    else {
        printf("Erreur : Le pseudo existe déjà \n");
  
    }
    return 0; //Erreur 
}

int logique_connexion( char *username, char *password) {
    char ligne[1024];
    int correspondance = 0;
    
    char password_hash[65];
    hash_password(password, password_hash);


    FILE *op = fopen("data/data_Joueur.csv", "r");
    if (op != NULL) {
        while(fgets(ligne, sizeof(ligne),op)  ){ //Condition d'arrêt : curseur arrive sur une ligne vide || username, mot de passe == pseudo_dans_csv, mtp  
            
            ligne[strcspn(ligne, "\r\n")] = 0; //permet de nettoyer chaque fin de ligne car prise en compte dans les strcmp
            char *pseudo_dans_csv = strtok(ligne, ",");
            char *password_dans_csv = strtok(NULL, ",");

            if( strcmp(username, pseudo_dans_csv) == 0 && strcmp(password_hash, password_dans_csv) == 0) {    
                correspondance = 1; //On a trouvé une correspondance, on peut créer la connexion 
                break;
            }
        }
        fclose(op);
    }

    if (correspondance == 0) { //cas aucune correspondance 

        printf("La connexion a échoué ! ");
        return 0; // Écriture réussie
    }
    else {
        printf("La connexion a réussi");
  
    }
    return 1; //Réussi :
}
