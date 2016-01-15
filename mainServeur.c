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
    char parametres[NBR_PARAM][TAILLE_PARAM];

    // variables d'authentification
    char login[TAILLE_LOGIN];
    char password[TAILLE_PASS];

    // variable de condition de la boucle de connexion
    int connecte = 0;

    // pointeur permettant pointer différents
    // endroits de la chaine de la requête.
    char* p_requete = NULL;

    // itérateur de boucle
    int i = 0;

    if(!Initialisation())
    {
        printf("[E] Une erreur est survenue lors de l'initialisation\n");
        exit(1);
    }

    while(1)
    {
        if(!(connecte = AttenteClient()))
        {
            printf("[E] Une erreur est survenue lors d'une connexion client !\n");
            exit(2);
        }

        while(connecte)
        {
            requete = Reception();

            printf("[R] Message reçu : %s\n", requete);

            printf("[+] Analyse de la requête...\n");
            parse_type(requete, type_requete);

            printf("[D] type_requete : %s\n", type_requete);

            // choix du comportement en fonction du type de requête
            if(!strncmp(type_requete, "authentification", strlen(type_requete)))
            {
                printf("[+] Demande d'authentification !\n");

                // on se place juste après le type de la requête
                // c'est à dire juste avant le login
                p_requete = strchr(requete, '/');
                if(p_requete == NULL)
                {
                    printf("[-] Erreur : Aucun paramètre détecté !\n");
                    exit(EXIT_FAILURE);
                }

                // on incrémente d'abord p_requete
                // pour qu'il ne soit plus sur le '/'
                // sur lequel on l'a placé avec strchr()
                p_requete++;
                // ensuite on peut lire l'utilisateur
                while(p_requete[i] != '/')
                {
                    login[i] = p_requete[i];
                    i++;
                }

                // permet de placer le pointeur p_requete
                // sur le '/' juste avant le mot de passe
                p_requete = strchr(p_requete, '/');

                // on incréente encore une fois pour
                // qu'il se place juste après le '/'
                p_requete++;
                printf("[D] p_requete = %s\n", p_requete);

                strncpy(password, p_requete, TAILLE_PASS);

                printf("[+] User : %s\n", login);
            }
        }

    }

    return 0;
}
