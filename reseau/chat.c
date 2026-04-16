#include <stdio.h>
#include <time.h>
#include "chat.h"
#include <stdlib.h>
#include <string.h>

// Anti - spam
#define DELAI_MESSAGE 5

// il s'agit de la fonction qui prepare les messgaes à envoyer
message * preparer_chat(char username[20] ,char text[200],time_t * derniermsgmoment){
    time_t now = time(NULL);
    // verification contre le spam
    if (difftime(now, *derniermsgmoment) < DELAI_MESSAGE) {
        return NULL;
    }
    message *tosend = malloc(sizeof(message));
    if (tosend == NULL){
        return NULL
    }

    struct tm *tm = localtime(&now);
    strncpy(tosend->msg, text,199);
    strncpy(tosend->username, username,19);
    tosend->momentenv.heure = (short)tm->tm_hour;
    tosend->momentenv.minute = (short)tm->tm_min;
    *derniermsgmoment = now;
    return tosend;
}
