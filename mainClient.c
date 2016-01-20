#include <stdio.h>
#include "client.h"
#include <stdlib.h>
int main(){
  char* machine=(char*)malloc(17);
  int ret;
  int choix=1;
  printf("rentrer @ :\n");
  fgets(machine,16,stdin);
  printf("%s",machine);
  machine[15]='\0';

  if(! Initialisation(machine)){// on traite le cas ou on a pas reussi la connexion au serveur
        printf("erreur de connexion au serveur ");
        exit(INTERN_ERROR);
  }

  do{
        ret=authentification();
        if (ret!=NO_PB){
            printf("voulez vous continuer ??? 1/yes 0/no ");
            scanf("%d",&choix);
            getchar();
        }

    }while(ret==AUTH_ERROR && choix);
    if(ret!=NO_PB)
      exit(AUTH_ERROR);
    /*on est authentifier et on procede a l'affichage du menu*/
    printf("la valeur recu est %d",ret);
    while (choix){
        printf("\n*************************************************************\n");
        printf("1:quitter\n");
        printf("\n*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
        printf("2:envoyer un message \n");
        printf("\n*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
        printf("3:consulter vos messages");
        printf("\n*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
        printf("en attente de votre choix: ");
        scanf("%d",&choix);
    }


    return 0;
}
