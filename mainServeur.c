#include <stdio.h>
#include <stdlib.h>

#include "serveur.h"


int main()
{
    char* requete = NULL;           // totalité de la requête (à parser)
    char* type_requete = NULL;      // type de la requête
    char** parametres = NULL;       // tableau vers les paramètres des requêtes

    if(!Initialisation())
    {
        printf("[E] Une erreur est survenue lors de l'initialisation\n");
        exit(1);
    }

    while(1)
    {
        if(!AttenteClient())
        {
            printf("[E] Une erreur est survenue lors d'une connexion client !\n");
            exit(2);
        }

        requete = Reception();

        printf("[R] Message reçu : %s\n", requete);

        printf("[+] Analyse de la requête...\n");
        parse_request(requete, type_requete, parametres);


    }

    return 0;
}
