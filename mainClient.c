#include <stdio.h>
#include "client.h"
int main(){
  int ret;
  int choix=1;
/*
  do{
        ret=authentification();
        if (ret!=NO_PB){
            printf("voulez vous continuer ??? 1/yes 0/no ");
            scanf("%d",&choix);
        }

    }while(ret==AUTH_ERROR && continuer);
    /*on est authentifier et on procede a l'affichage du menu*/
    while (choix){
        printf("\n*************************************************************\n");
        printf("1:quitter\n");
        printf("\n*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
        printf("2:envoyer un message \n");
        printf("\n*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
        printf("3:consulter vos messages");
        printf("\n*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
        printf("en attente de votre choix: ");
        scanf("%d",choix);
    }


    return 0;
}
