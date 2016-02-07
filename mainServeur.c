#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "serveur.h"

int main(void)
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
            if(requete == NULL)
            {
                printf("[+] Erreur : fatal socket error !\n");
                envoi_reponse(SERV_ERROR);
                exit(EXIT_FAILURE);
            }

            printf("[+] Analyse de la requête...\n");
            parseType(requete, type_requete);

            printf("[D] type_requete : %s\n", type_requete);

            if(strncmp(type_requete, "authentification", strlen(type_requete)) == 0)
            {
                printf("[+] Demande d'authentification\n");

                authentifie = authentification(requete, login, password);
            }
            else if(strncmp(type_requete, "send", strlen(type_requete)) == 0)
            {
                printf("[+] Demande d'envoi d'un message\n");

                if(sendMessage(requete) == SERV_ERROR)
                {
                    printf("[-] Exiting !\n");
                    exit(EXIT_FAILURE);
                }
            }
            else if(strncmp(type_requete, "check", strlen(type_requete)) == 0)
            {
                printf("[+] Demande du nombre de messages non lus\n");

                checkNewMessages(login);
            }
            else if(strncmp(type_requete, "list", strlen(type_requete)) == 0)
            {
                printf("[+] Demande de listing\n");

                listMessages(requete, login);
            }
            else if(strncmp(type_requete, "read", strlen(type_requete)) == 0)
            {
                printf("[+] Demande de lecture de message\n");

                readMessage(requete, login);
            }
            else if((strncmp(type_requete, "delete", strlen(type_requete)) == 0))
            {
                printf("[+] Demande de suppression de message\n");

                deleteMessage(requete, login);
            }
            else
            {
                printf("[+] Erreur : trame non reconnue !\n");
                envoi_reponse(SERV_ERROR);
            }

        }

    }

    return 0;
}
