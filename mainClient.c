#include <stdio.h>
#include "client.h"
#include <stdlib.h>
int main(){
    char principal='a';
    char login[TAILLE_ID];
    char machine[17];
    int ret;
    char continuer ='y';
    char choix='b';//initialisation pour le premier tour de boucle
    printf("rentrer @ :\n");
    fgets(machine,16,stdin);// @ip

    machine[15]='\0';
    //viderBuffer();
    system("clear");// nettoie le terminal

    if(! Initialisation(machine) ){// on traite le cas ou on a pas reussi la connexion au serveur
        printf("erreur de connexion au serveur \n   ");
        exit(INTERN_ERROR);
    }

    while (principal != '3'){


        afficher_menu();
        principal=getchar();
        viderBuffer();

        if(principal == '1'){
            // l'authentification
            do{

                if ( continuer == 'n' )/*si jamais l'utilisateur ne veut pas se reauthentifier*/

                    exit(AUTH_ERROR);

                    ret=authentification(login);
                    printf("[D] LOGIN = %s RET = %d\n", login, ret);

                    if ( ret != NO_PB ){

                    printf("*********erreur d'authentification *********\n");
                    printf("voulez vous continuer ??? y/yes n/no\n ");
                    continuer=fgetc(stdin);
                    viderBuffer();

                }
                /*if (ret == SERV_ERROR){
                    printf("une erreur s'est produite\n");
                    exit(SERV_ERROR);
                }
                printf("ret vau %d",ret);*/

            }while( (ret == AUTH_ERROR ) && continuer == 'y' );

            if(ret != NO_PB ){
                printf("++++++++++++++++a bientot!!+++++++++++++++++\n");
                Terminaison();
                exit(AUTH_ERROR);
            }

            system("clear");

            /*une fois ici on est authentifier et on procede a l'affichage du menu*/

            afficher_menu1();
            //viderBuffer();
            choix=fgetc(stdin);
            viderBuffer();
            while ( choix !='a' ){// tant que le client ne veut pas quitter
                //system("clear");
                if ( choix == 'b' ){

                    Envoyermessage(login);
                    afficher_menu1();
                    choix=fgetc(stdin);

                }

                else if( choix == 'c' ){
                    //system("clear");
                    afficher_menu2();

                    //viderBuffer();
                    choix=fgetc(stdin);

                    //system("clear");
                }
                else if ( choix == 'd' ){
                        check();
                        afficher_menu2();
                        choix=fgetc(stdin);

                }
                else if (choix == 'e'){
                    list("new");
                    afficher_menu2();
                    choix=fgetc(stdin);

                }
                else if(choix == 'f'){
                    lire();
                    afficher_menu2();
                    choix=fgetc(stdin);

                }
                else if (choix == 'g'){
                    list("all");
                    afficher_menu2();
                    choix=fgetc(stdin);
                }
                else if (choix== 'h'){
                    supprimer();
                    afficher_menu2();
                    choix=getchar();
                }
                else if( choix == 'i' ){
                        afficher_menu1();
                        choix=fgetc(stdin);
                }
                else {
                    printf("Attention vous avez choisis une option n'existe pas ,veuillez recommencer\n");
                    afficher_menu1();
                    choix=fgetc(stdin);

                }

                viderBuffer();
                system("clear");



            }
            /*une fois, ici le client veut se deconnecté
            deconnexion
            */
            deconnexion();
        }
        else if( principal == '2' ){
            inscription();
        }
        else{
            Terminaison();// fermé la connexion
            return 1;

        }
    }
    return 0;
}
