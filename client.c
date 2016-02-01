#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <crypt.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#endif

#include <errno.h>

#ifdef WIN32
#define perror(x) printf("%s : code d'erreur : %d\n", (x), WSAGetLastError())
#define close closesocket
#define socklen_t int
#endif

#include "client.h"

#define TRUE 1
#define FALSE 0

#define LONGUEUR_TAMPON 4096

/* Variables cachees */

/* le socket client */
int socketClient;
/* le tampon de reception */
char tamponClient[LONGUEUR_TAMPON];
int debutTampon;
int finTampon;
int finConnexion = FALSE;

/* Initialisation.
 * Connexion au serveur sur la machine donnee.
 * Utilisez localhost pour un fonctionnement local.
 */
int Initialisation(char *machine) {
	return InitialisationAvecService(machine, "13214");
}

/* Initialisation.
 * Connexion au serveur sur la machine donnee et au service donne.
 * Utilisez localhost pour un fonctionnement local.
 */
int InitialisationAvecService(char *machine, char *service) {
	int n;
	struct addrinfo	hints, *res, *ressave;

	#ifdef WIN32
	WSADATA	wsaData;
	if (WSAStartup(0x202,&wsaData) == SOCKET_ERROR)
	{
		printf("WSAStartup() n'a pas fonctionne, erreur : %d\n", WSAGetLastError()) ;
		WSACleanup();
		exit(1);
	}
	memset(&hints, 0, sizeof(struct addrinfo));
    #else
	bzero(&hints, sizeof(struct addrinfo));
	#endif
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(machine, service, &hints, &res)) != 0)  {
     		fprintf(stderr, "Initialisation, erreur de getaddrinfo : %s", gai_strerror(n));
     		return 0;
	}
	ressave = res;

	do {
		socketClient = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (socketClient < 0)
			continue;	/* ignore this one */

		if (connect(socketClient, res->ai_addr, res->ai_addrlen) == 0)
			break;		/* success */

		close(socketClient);	/* ignore this one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL) {
     		perror("Initialisation, erreur de connect.");
     		return 0;
	}

	freeaddrinfo(ressave);

	finConnexion = FALSE;

	printf("Connexion avec le serveur reussie.\n");

	return 1;
}

/* Recoit un message envoye par le serveur.
 */
char *Reception() {
	char message[LONGUEUR_TAMPON];
	int index = 0;
	int fini = FALSE;
	int retour = 0;
	int trouve = FALSE;

	if(finConnexion) {
		return NULL;
	}

	while(!fini) {
		/* on cherche dans le tampon courant */
		while((finTampon > debutTampon) && (!trouve)) {
			//fprintf(stderr, "Boucle recherche char : %c(%x), index %d debut tampon %d.\n",
			//					tamponClient[debutTampon], tamponClient[debutTampon], index, debutTampon);
			if (tamponClient[debutTampon]=='/' && tamponClient[debutTampon+1]==';')
				trouve = TRUE;
			else
				message[index++] = tamponClient[debutTampon++];
		}
		/* on a trouve ? */
		if (trouve) {
			//message[index++] = '\n';
			// on recopie le /; dans le message
			message[index++] = tamponClient[debutTampon++];
			message[index++] = tamponClient[debutTampon++];
			message[index] = '\0';
			debutTampon++;
			fini = TRUE;
			//fprintf(stderr, "trouve\n");
#ifdef WIN32
			return _strdup(message);
#else
			return strdup(message);
#endif
		} else {
			/* il faut en lire plus */
			debutTampon = 0;
			//fprintf(stderr, "recv\n");
			retour = recv(socketClient, tamponClient, LONGUEUR_TAMPON, 0);
			//fprintf(stderr, "retour : %d\n", retour);
			if (retour < 0) {
				perror("Reception, erreur de recv.");
				return NULL;
			} else if(retour == 0) {
				fprintf(stderr, "Reception, le serveur a ferme la connexion.\n");
				finConnexion = TRUE;
				// on n'en recevra pas plus, on renvoie ce qu'on avait ou null sinon
				if(index > 0) {
					message[index++] = '\n';
					message[index] = '\0';
#ifdef WIN32
					return _strdup(message);
#else
					return strdup(message);
#endif
				} else {
					return NULL;
				}
			} else {
				/*
				 * on a recu "retour" octets
				 */
				finTampon = retour;
			}
		}
	}
	return NULL;
}

/* Envoie un message au serveur.
 * Attention, le message doit etre termine par \n
 */
int Emission(char *message) {
	int taille;
	if(strstr(message, "/;") == NULL) {
		fprintf(stderr, "Emission, Le message n'est pas termine par \\n.\n");
	}
	taille = strlen(message);
	if (send(socketClient, message, taille,0) == -1) {
        perror("Emission, probleme lors du send.");
        return 0;
	}
	printf("Emission de %d caracteres.\n", taille+1);
	return 1;
}

/* Recoit des donnees envoyees par le serveur.
 */
int ReceptionBinaire(char *donnees, size_t tailleMax) {
	size_t dejaRecu = 0;
	int retour = 0;
	/* on commence par recopier tout ce qui reste dans le tampon
	 */
	while((finTampon > debutTampon) && (dejaRecu < tailleMax)) {
		donnees[dejaRecu] = tamponClient[debutTampon];
		dejaRecu++;
		debutTampon++;
	}
	/* si on n'est pas arrive au max
	 * on essaie de recevoir plus de donnees
	 */
	if(dejaRecu < tailleMax) {
		retour = recv(socketClient, donnees + dejaRecu, tailleMax - dejaRecu, 0);
		if(retour < 0) {
			perror("ReceptionBinaire, erreur de recv.");
			return -1;
		} else if(retour == 0) {
			fprintf(stderr, "ReceptionBinaire, le serveur a ferme la connexion.\n");
			return 0;
		} else {
			/*
			 * on a recu "retour" octets en plus
			 */
			return dejaRecu + retour;
		}
	} else {
		return dejaRecu;
	}
}

/* Envoie des donnees au serveur en precisant leur taille.
 */
int EmissionBinaire(char *donnees, size_t taille) {
	int retour = 0;
	retour = send(socketClient, donnees, taille, 0);
	if(retour == -1) {
		perror("Emission, probleme lors du send.");
		return -1;
	} else {
		return retour;
	}
}


/* Ferme la connexion.
 */
void Terminaison() {
	close(socketClient);
}
/*vider le buffer
*/
void viderBuffer()
{
    int c = 0;
    while (c != '\n' && c != EOF)
    {
        c = getchar();
    }
}
/*teste de malloc
*/
void teste_malloc(char *ptr){

	if (ptr==NULL){
		printf("une erreur s'est produite!!! erreur de memoire\n");
		exit(INTERN_ERROR);
	}
}


/*authentification

*/
int authentification(char *login){
	int code_ret;
	char* request=(char*)malloc(TAILLE_REQUETTE);

	teste_malloc(request);

	char * password=(char*)malloc(TAILLE_PASSWORD);
	teste_malloc(password);
	/********************************************************/

	printf("bienvenue sur votre messagerie\n !!");
	printf("authentifiez vous!!\n");
	printf("login: ");
	fgets(login,TAILLE_ID,stdin);
	login[strlen(login)-1]='\0';/*elimination du retour a la ligne*/
	printf("password: ");
	fgets(password,TAILLE_ID,stdin);
	password[strlen(password)-1]='\0';/*elimination du retour a la ligne*/
	//password = getpass("password:");
	/*cryptage du mot de passe avant l'envoie*/
	strncpy(password,crypt(password,"$6$"),TAILLE_PASSWORD);

	/*formulation de la requette*/

	sprintf(request,"authentification/%s/%s/;",login,password);


	/*envoie de la requette*/
	printf("la requette %s\n",request);

	Emission(request);

	/*attente de la reponse*/
	request=Reception();

	if (request!=NULL){
		sscanf(request,"return/%d/;",&code_ret);
		return (code_ret);
    }
	else
		return(INTERN_ERROR);

}
/*ce sous programme prend en paramettre @ de l'expediteur
 recuperera les champs necessaire pour l'envoie d'un message
*/
void Envoyermessage(char login[]){

	int continuer=0;
	char dest[TAILLE_ID];
	char objet[TAILLE_PASSWORD];
	char contenu[TAILLE_CONETENU];
	char *request=(char *)malloc(TAILLE_REQUETTE);

 	teste_malloc(request);


	int code_ret;

	viderBuffer();
	printf("veuillez saisir l'@ destinatrice: ");
	fgets(dest,TAILLE_ID,stdin);
	dest[strlen(dest)-1]='\0';

	printf("veuillez saisir objet: ");
	fgets(objet,TAILLE_PASSWORD,stdin);
	objet[strlen(objet)-1]='\0';


	printf("veuillez saisir votre message : ");
	fgets(contenu,TAILLE_CONETENU,stdin);
	contenu[strlen(contenu)-1]='\0';
	/*formulation de la requette*/
	sprintf(request,"send/%s/%s/%s/%s/;",login,dest,objet,contenu);
    /*envoie de la requette*/

	Emission(request);

	/*reception de la requette*/

	request=Reception();

	if (request!=NULL)
		sscanf(request,"return/%d/;",&code_ret);
	else
		exit(INTERN_ERROR);

	if (code_ret==SERV_ERROR){
		printf("erreur de serveur!!! reessayer ulterieurement");
		exit(SERV_ERROR);
	}


		if(code_ret== DEST_ERROR){

			do{
				printf("vous vous etes tropmé de destinataire voulez vous reessayer? o/oui n/non ");
				continuer=fgetc(stdin);

				printf(" veuillez ressaisir la bonne adresse :\n");
				fgets(dest,TAILLE_ID,stdin);
				dest[strlen(dest)-1]='\0';

				bzero(request,TAILLE_REQUETTE);//on vide la chaine de charactere

				/*formulation de la requette*/
				sprintf(request,"send/%s/%s/%s/%s/;",login,dest,objet,contenu);

				Emission(request);

				request=Reception();

				if (request!=NULL)
					sscanf(request,"return/%d/;",&code_ret);
				else
					exit(INTERN_ERROR);

			}while(continuer=='y' && code_ret!=DEST_ERROR);
			printf("message envoyé avec succès\n");
		}

}
/*deconnexion
*/
void deconnexion(){

	char *request=(char*)malloc(TAILLE_REQUETTE);
	teste_malloc(request);
	strcat(request,"disc/;");
	Emission(request);
}

/*le sous programme se chargera
 *d'envoyer une requette au serveur pour savoir si
 * il y'a eu des nouveaux messages ou pas
*/

void check(){

	char*request=(char*)malloc(TAILLE_REQUETTE);
	teste_malloc(request);
	int code_ret;
	strcat(request,"check/;");
	Emission(request);

	request = Reception();

	if (request!=NULL){

		sscanf(request,"check/%d/;",&code_ret);

    }
	else
		exit(INTERN_ERROR);


	printf("vous avez %d nouveaux message\n",code_ret);
}
/*affiche le menu principale
*/
void afficher_menu1(){
	printf("*************************************************************\n");
	printf("a:deconnexion\n");
	printf("*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
	printf("b:envoyer un message \n");
	printf("*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
	printf("c:consulter vos messages\n");
	printf("*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
	printf("en attente de votre choix: ");
}
/*affiche le menu secondaire
*/
void afficher_menu2(){
	printf("\n*************************************************************\n");
	printf("d:ya t-il des nouveaux message ?\n");
	printf("*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
	printf("e:liste des nouveaux messages \n");
	printf("*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
	printf("f:lire un message \n");
	printf("*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
	printf("g:liste de tout les messages \n");
	printf("*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
	printf("h:retourner au menu d'avant \n");
	printf("*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
	printf("en attente de votre choix: ");
}
