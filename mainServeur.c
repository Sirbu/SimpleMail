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
    // 0 si il n'est pas authentifié
    // 1 si il est authentifié.
    int authentifie = 0;

    if(!Initialisation())
    {
        printf("[-] Une erreur est survenue lors de l'initialisation\n");
        exit(1);
    }

    while(1)
    {
        printf("\n[+] En attente de connexion d'un client...\n");

        if(!(connecte = AttenteClient()))
        {
            printf("[-] Erreur : la connection du client a échoué !\n");
            exit(2);
        }

        while(connecte)
        {
            requete = Reception();

            printf("Message reçu : %s\n:", requete);
            if(requete == NULL)
            {
                printf("[-] Warning : connexion interrompue !\n");
                authentifie = 0;
                connecte = 0;
            }
            else
            {
                printf("[+] Analyse de la requête...\n");
                parseType(requete, type_requete);

                // printf("[D] type_requete : %s\n", type_requete);

                // if en série pour déterminer l'action à effectuer
                // en fonction du type de la requête
                if(!authentifie)
                {
                    // les requêtes suivantes sont les seules a être
                    // autorisées lorsque l'utilisateur n'est pas
                    // encore logué
                    if(strncmp(type_requete, "authentification", strlen(type_requete)) == 0)
                    {
                        printf("\n##### Demande d'authentification #####\n");

                        authentifie = authentification(requete, login, password);
                    }
                    else if((strncmp(type_requete, "inscription", strlen(type_requete)) == 0))
                    {
                        printf("##### Demande d'inscription #####\n");

                        if(inscription(requete) != 0)
                        {
                            fprintf(stderr, "[-] Erreur : Inscription impossible !\n");
                            envoi_reponse(AUTH_ERROR);
                        }
                        else
                        {
                            envoi_reponse(NO_PB);
                        }

                    }
                    else
                    {
                        fprintf(stderr, "[-] Erreur : authentification requise !\n");
                        envoi_reponse(AUTH_ERROR);
                    }
                }
                else // si on est authentifié, on peut accepter d'autres requêtes
                {
                    if(strncmp(type_requete, "send", strlen(type_requete)) == 0)
                    {
                        printf("\n##### Demande d'envoi d'un message #####\n");

                        if(sendMessage(requete) == SERV_ERROR)
                        {
                            fprintf(stderr, "[-] Erreur : Envoi du message impossible !\n");
                            Terminaison();
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if(strncmp(type_requete, "check", strlen(type_requete)) == 0)
                    {
                        printf("\n##### Demande du nombre de messages non lus #####\n");

                        if(checkNewMessages(login) != 0)
                        {
                            fprintf(stderr, "[-] Erreur : vérification messages non lus impossible\n");
                            Terminaison();
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if(strncmp(type_requete, "list", strlen(type_requete)) == 0)
                    {
                        printf("\n##### Demande de listing #####\n");

                        if(listMessages(requete, login) != 0)
                        {
                            fprintf(stderr, "[-] Erreur : listing des messages impossible\n");
                            Terminaison();
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if(strncmp(type_requete, "read", strlen(type_requete)) == 0)
                    {
                        printf("\n##### Demande de lecture de message #####\n");

                        if(readMessage(requete, login) != 0)
                        {
                            fprintf(stderr, "[-] Erreur : Lecture du message impossible\n");
                            Terminaison();
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if((strncmp(type_requete, "delete", strlen(type_requete)) == 0))
                    {
                        printf("\n##### Demande de suppression de message #####\n");

                        if(deleteMessage(requete, login) != 0)
                        {
                            fprintf(stderr, "[-] Erreur : Suppression du message impossible\n");
                            Terminaison();
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if((strncmp(type_requete, "disconnect", strlen(type_requete)) == 0))
                    {
                        printf("\n##### Demande de déconnexion ! #####\n");

                        authentifie = 0;
                        printf("[+] Déconnexion aceptée : Au revoir %s\n", login);

                        bzero(login, TAILLE_LOGIN);
                        bzero(password, TAILLE_PASS);

                        // je n'envoie pas de trame particulière car
                        // si je le fais, elle sera lue au prochain appel
                        // à Reception que fera le client. Et c'est pour
                        // l'authentification qu'il y fait appel...
                    }
                    else
                    {
                        printf("[-] Erreur : trame non reconnue !\n");
                        envoi_reponse(SERV_ERROR);
                    }

                }
            }


            free(requete);
        }

    }

    return 0;
}
