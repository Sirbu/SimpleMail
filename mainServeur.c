#include <stdio.h>
#include <stdlib.h>

#include "serveur.h"

int main()
{
    char* buffer_reception;

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

        buffer_reception = Reception();

        printf("[R] Message reçu : %s\n", buffer_reception);

    }



    return 0;
}
