#include <stdio.h>
#include "client.h"
#include <stdlib.h>
int main(){
    char login[TAILLE_ID];
    char machine[17];
    int ret;
    int continuer =0;
    char choix='y';
    printf("rentrer @ :\n");
    fgets(machine,16,stdin);
    printf("%s",machine);
    machine[15]='\0';

    if(! Initialisation(machine)){// on traite le cas ou on a pas reussi la connexion au serveur
        printf("erreur de connexion au serveur ");
        exit(INTERN_ERROR);
    }

    do{
        if (choix=='n')/*si jamais l'utilisateur ne veut pas se reauthentifier*/
            exit(AUTH_ERROR);
            printf("tttttttttttttttttttttttttttttt\n" );
        ret=authentification(login);
        if (ret!=NO_PB){
            printf("***erreur d'authentification \n***");
            printf("voulez vous continuer ??? y/yes n/no ");
            choix=fgetc(stdin);
            getchar();
        }

    }while(ret==AUTH_ERROR && choix=='y');

    if(ret!=NO_PB)
      exit(AUTH_ERROR);


    /*une fois ici on est authentifier et on procede a l'affichage du menu*/

    while (choix !='a'){
        printf("\n*************************************************************\n");
        printf("a:quitter\n");
        printf("\n*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
        printf("b:envoyer un message \n");
        printf("\n*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
        printf("c:consulter vos messages");
        printf("\n*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
        printf("en attente de votre choix: ");

        choix=fgetc(stdin);

        if (choix=='b'){
            Envoyermessage(login);

        }

    }
    return 0;
}
