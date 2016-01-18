#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "serveur.h"


int main()
{
    // totalité de la requête (à parser)
    // c'est un char* pour être compatible
    // avec la fonction Reception()
    char* requete = NULL;

     // type de la requête
    char type_requete[TAILLE_TYPE];

    // tableau vers les paramètres des requêtes
    // char parametres[NBR_PARAM][TAILLE_PARAM];

    // variables d'authentification
    char login[TAILLE_LOGIN];
    char password[TAILLE_PASS];

    // variable de condition de la boucle de connexion
    int connecte = 0;

    // Permet de savoir si les requêtes que l'on reçoit
    // viennent d'un utilisateur authentifié ou non.
    // 0 si il n'est pas authentifié et 1 sinon.
    int authentifie = 0;

    if(!Initialisation())
    {
        printf("[-] Une erreur est survenue lors de l'initialisation\n");
        exit(1);
    }

    while(1)
    {
        if(!(connecte = AttenteClient()))
        {
            printf("[-] Erreur : la connection du client a échoué !\n");
            exit(2);
        }

        while(connecte)
        {
            requete = Reception();

            printf("[+] Message reçu : %s\n", requete);

            printf("[+] Analyse de la requête...\n");
            parseType(requete, type_requete);

            printf("[D] type_requete : %s\n", type_requete);

            // choix du comportement en fonction du type de requête
            if(strncmp(type_requete, "authentification", strlen(type_requete)) == 0)
            {
                printf("[+] Demande d'authentification !\n");

                if(parseLoginPass(requete, login, password))
                {
                    fprintf(stderr, "[-] Erreur : Extraction des informations d'authentification impossible !\n");

                    envoi_reponse(AUTH_ERROR);

                    exit(EXIT_FAILURE);
                }

                if(checkAuthentification(login, password) == 0)
                {
                    printf("[+] Authentification validée !\n");
                    printf("[+] Bienvenue %s!\n", login);

                    envoi_reponse(NO_PB);

                    authentifie = 1;
                }
            }
        }

    }

    return 0;
}
