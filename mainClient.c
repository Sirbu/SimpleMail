#include <stdio.h>
#include "client.h"
#include <stdlib.h>
int main(){
    char login[TAILLE_ID];
    char machine[17];
    int ret;
    char continuer ='y';
    char choix='b';//initialisation pour le premier tour de boucle
    printf("rentrer @ :\n");
    fgets(machine,16,stdin);
    printf("%s",machine);
    machine[15]='\0';


    if(! Initialisation(machine)){// on traite le cas ou on a pas reussi la connexion au serveur
        printf("erreur de connexion au serveur ");
        exit(INTERN_ERROR);
    }

    do{

        if (continuer=='n')/*si jamais l'utilisateur ne veut pas se reauthentifier*/
            exit(AUTH_ERROR);

        ret=authentification(login);

        if (ret!=NO_PB){

            printf("***erreur d'authentification \n***");
            printf("voulez vous continuer ??? y/yes n/no ");
            continuer=fgetc(stdin);
            viderBuffer();

        }

    }while(ret==AUTH_ERROR && continuer=='y');

    if(ret!=NO_PB)
      exit(AUTH_ERROR);

    //system("clear");
    /*une fois ici on est authentifier et on procede a l'affichage du menu*/

    while (choix !='a'){
        if ( choix == 'b' || choix == 'c' || choix == 'h' ){
            afficher_menu1();

            choix=fgetc(stdin);

            //system("clear");
            printf("j'ai choisis.... %c\n",choix);
        }
        if ( choix == 'a' ){

            printf("merci d'avoir utliser Mehdi&Scou !!");

            deconnexion();

        }
        else if ( choix == 'b' ){

            Envoyermessage(login);

        }

        else if( choix == 'c' ){
            //system("clear");
            afficher_menu2();

            viderBuffer();

            choix=fgetc(stdin);
            //system("clear");
            if ( choix =='d' ){
                check();
                afficher_menu2();

                viderBuffer();

                choix=fgetc(stdin);
            }
            else if( choix == 'h' ){
                choix='w';//je met une valeur choisis pour ne pas reboucler
            }

        }
    }
    return 0;
}
