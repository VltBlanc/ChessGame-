#include <stdio.h> //FILE
#include <stdlib.h>
#include <string.h>




int logique_create_account( char *username, char *password) {


    //Précondition : Taille des entrées 
    if (strlen(username) >= 32 || strlen(password) >= 32) {
        printf("Erreur : Pseudo ou mot de passe trop long.\n");
        return 0;
    }

    char ligne[1024];
    int existe_deja = 0;


    FILE *op = fopen("../data/data_Joueur.csv", "r");
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
        FILE *op = fopen("../data/data_Joueur.csv", "a");
        fprintf(op,"%s,%s\n", username,password);
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



    FILE *op = fopen("../data/data_Joueur.csv", "r");
    if (op != NULL) {
        while(fgets(ligne, sizeof(ligne),op)  ){ //Condition d'arrêt : curseur arrive sur une ligne vide || username, mot de passe == pseudo_dans_csv, mtp  
            
            ligne[strcspn(ligne, "\r\n")] = 0; //permet de nettoyer chaque fin de ligne car prise en compte dans les strcmp
            char *pseudo_dans_csv = strtok(ligne, ",");
            char *password_dans_csv = strtok(NULL, ",");

            if( strcmp(username, pseudo_dans_csv) == 0 && strcmp(password,password_dans_csv) == 0) {    
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



void* create_account() {
    char username[32];
    char password[32];

    printf("username : ");
    scanf("%s", username); 
    printf("password : ");
    scanf("%s", password);

    logique_create_account( username,password);
}

void* connexion() {
    char username[32];
    char password[32];

    printf("username : ");
    scanf("%s", username); 
    printf("password : ");
    scanf("%s", password);
    logique_connexion(username,password);
    
    return 0;
}