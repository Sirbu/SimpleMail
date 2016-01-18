#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>

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

#include <crypt.h>
#include "serveur.h"

#define TRUE 1
#define FALSE 0
#define LONGUEUR_TAMPON 4096


#ifdef WIN32
#define perror(x) printf("%s : code d'erreur : %d\n", (x), WSAGetLastError())
#define close closesocket
#define socklen_t int
#endif

/* Variables cachees */

/* le socket d'ecoute */
int socketEcoute;
/* longueur de l'adresse */
socklen_t longeurAdr;
/* le socket de service */
int socketService;
/* le tampon de reception */
char tamponClient[LONGUEUR_TAMPON];
int debutTampon;
int finTampon;
int finConnexion = 0;


/* Initialisation.
 * Creation du serveur.
 */
int Initialisation() {
	return InitialisationAvecService("13214");
}

/* Initialisation.
 * Creation du serveur en precisant le service ou numero de port.
 * renvoie 1 si ca c'est bien passe 0 sinon
 */
int InitialisationAvecService(char *service) {
	int n;
	const int on = 1;
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

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(NULL, service, &hints, &res)) != 0)  {
     		printf("Initialisation, erreur de getaddrinfo : %s", gai_strerror(n));
     		return 0;
	}
	ressave = res;

	do {
		socketEcoute = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (socketEcoute < 0)
			continue;		/* error, try next one */

		setsockopt(socketEcoute, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
#ifdef BSD
		setsockopt(socketEcoute, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
#endif
		if (bind(socketEcoute, res->ai_addr, res->ai_addrlen) == 0)
			break;			/* success */

		close(socketEcoute);	/* bind error, close and try next one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL) {
     		perror("Initialisation, erreur de bind.");
     		return 0;
	}

	/* conserve la longueur de l'addresse */
	longeurAdr = res->ai_addrlen;

	freeaddrinfo(ressave);
	/* attends au max 4 clients */
	listen(socketEcoute, 4);
	printf("Creation du serveur reussie sur le port %s\n", service);

	return 1;
}

/* Attends qu'un client se connecte.
 */
int AttenteClient() {
	struct sockaddr *clientAddr;
	char machine[NI_MAXHOST];

	clientAddr = (struct sockaddr*) malloc(longeurAdr);
	socketService = accept(socketEcoute, clientAddr, &longeurAdr);
	if (socketService == -1) {
		perror("AttenteClient, erreur de accept.");
		return 0;
	}
	if(getnameinfo(clientAddr, longeurAdr, machine, NI_MAXHOST, NULL, 0, 0) == 0) {
		printf("Client sur la machine d'adresse %s connecte.\n", machine);
	} else {
		printf("Client anonyme connecte.\n");
	}
	free(clientAddr);
	/*
	 * Reinit buffer
	 */
	debutTampon = 0;
	finTampon = 0;
	finConnexion = FALSE;

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
			//		tamponClient[debutTampon], tamponClient[debutTampon], index, debutTampon);
			if (tamponClient[debutTampon]=='/'
				&& tamponClient[debutTampon+1] == ';')
				trouve = TRUE;
			else
				message[index++] = tamponClient[debutTampon++];
		}
		/* on a trouve ? */
		if (trouve) {
			// message[index++] = '\n';
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
			retour = recv(socketService, tamponClient, LONGUEUR_TAMPON, 0);
			//fprintf(stderr, "retour : %d\n", retour);
			if (retour < 0) {
				perror("Reception, erreur de recv.");
				return NULL;
			} else if(retour == 0) {
				fprintf(stderr, "Reception, le client a ferme la connexion.\n");
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

/* Envoie un message au client.
 * Attention, le message doit etre termine par \n
 */
int Emission(char *message) {
	int taille;
	if(strstr(message, "/;") == NULL) {
		fprintf(stderr, "[-] Erreur : Emission, Le message n'est pas termine par \'/;\'.\n");
	}
	taille = strlen(message);
	if (send(socketService, message, taille,0) == -1) {
        perror("[-] Erreur : Emission, probleme lors du send.");
        return 0;
	}
	printf("[+] Emission de %d caracteres.\n", taille+1);
	return 1;
}


/* Recoit des donnees envoyees par le client.
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
		retour = recv(socketService, donnees + dejaRecu, tailleMax - dejaRecu, 0);
		if(retour < 0) {
			perror("ReceptionBinaire, erreur de recv.");
			return -1;
		} else if(retour == 0) {
			fprintf(stderr, "ReceptionBinaire, le client a ferme la connexion.\n");
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

/* Envoie des donnees au client en precisant leur taille.
 */
int EmissionBinaire(char *donnees, size_t taille) {
	int retour = 0;
	retour = send(socketService, donnees, taille, 0);
	if(retour == -1) {
		perror("[-] Erreur : Emission, problème lors du send.");
		return -1;
	} else {
		return retour;
	}
}



/* Ferme la connexion avec le client.
 */
void TerminaisonClient() {
	close(socketService);
}

/* Arrete le serveur.
 */
void Terminaison() {
	close(socketEcoute);
}

// Fonctions écrites par les élèves

// Permet de parser la requête pour stocker
// les différents champs
// retourne 0 si tout se passe bien, un code d'erreur sinon.
int parseType(char* requete, char* type_requete)
{
	char* p_type = NULL; 	// pointeur qui sera placé juste après le champ type
	int i = 0; 			   // compteur de boucle...

	if(requete == NULL)
	{
		fprintf(stderr, "[-] Parsing error : la chaine requête est nulle !\n");
		return 1;
	}

	// si la requête n'a pas de paramètres, le type
	// est donc la seule chose présente dans la requête
	printf("[+] Extraction du type de la requête\n");
	p_type = strchr(requete, '/');
	if(p_type == NULL)
	{
		fprintf(stderr, "[-] Warning : carcactère \'/\' absent de la requête\n");

		strncpy(type_requete, requete, TAILLE_TYPE);

		return 2;
	}
	else
	{
		while(requete[i] != '/')
		{
			type_requete[i] = requete[i];
			i++;
		}
		type_requete[i] = '\0';
	}

	return 0;	// aucune erreur
}

// retourne un code d'erreur si il y a un problème
// et 0 sinon
int parseLoginPass(char* requete, char* login, char* password)
{
	// pointeur permettant pointer différents
	// endroits de la chaine de la requête.
	char* p_requete = NULL;

	// itérateur de boucle
	int i = 0;

	// on se place juste après le type de la requête
	// c'est à dire juste avant le login
	p_requete = strchr(requete, '/');
	if(p_requete == NULL)
	{
		fprintf(stderr, "[-] Erreur : Aucun paramètre détecté !\n");
		return 1;
	}

	// on incrémente d'abord p_requete
	// pour qu'il ne soit plus sur le '/'
	// sur lequel on l'a placé avec strchr()
	p_requete++;
	// ensuite on peut lire l'utilisateur
	while(p_requete[i] != '/')
	{
		login[i] = p_requete[i++];
	}

	// permet de placer le pointeur p_requete
	// sur le '/' juste avant le mot de passe
	p_requete = strchr(p_requete, '/');

	// on incrémente encore une fois pour
	// qu'il se place juste après le '/'
	p_requete++;

	strncpy(password, p_requete, TAILLE_PASS);

	printf("[+] User : %s length = %d\n", login, (int)strlen(login));
	printf("[D] Password : %s\n", password);

	return 0;
}

// vérifie les informations d'authentifcation en paramètres.
// Attention, le mot de passe est un hash contenant l'id
// de l'algorithme, le sel et le mot de passe. Le sel est une
// chaine vide, et chaque champ est séparé par un '$' comme suit :
// $id$salt$hash
// Retourne 0 si tout s'est bien passé.
// 1 si le login ou le mot de passe n'est pas bon
int checkAuthentification(char* login, char* password)
{
	// permettra de stocker une ligne du fichier
	char line[LINE_LENGTH];

	// détermine si on a trouvé le login dans le fichier
	int found = 0;

	// pointeur placé à la jonction dans la ligne
	// entre login et password
	char* p_pass = NULL;

	FILE* auth_file = fopen("bdd", "r");
	if(auth_file == NULL)
	{
		fprintf(stderr, "[-] Error : Impossible to open the database file !\n");
		return 1;
	}

	while(!found && fgets(line, LINE_LENGTH, auth_file) != NULL)
	{
		printf("[D] login = %s\nLine = %s\n", login, line);
		if(strstr(line, login) != NULL)
		{
			found = 1;
		}
	}

	if(!found)
	{
		// si le login n'est pas trouvé dans le fichier...
		fprintf(stderr, "[-] Erreur : login %s introuvable !\n", login);
		fclose(auth_file);
		return 1;
	}

	// permet d'enlever le newline qui fait chier
	line[strlen(line)-1] = '\0';

	p_pass = strchr(line, ':');
	if(p_pass == NULL)
	{
		fprintf(stderr, "[-] Erreur : Fichier d'authentification corrompu !\n");
		fclose(auth_file);
		exit(EXIT_FAILURE);
	}

	fclose(auth_file);

	// on retourne le réultat de la comparaison
	// du hash reçu avec celui dans le fichier
	p_pass++;
	printf("[D] p_pass = %s\npassword = %s\n", p_pass, password);
	return strncmp(p_pass, password, strlen(p_pass));
}

void envoi_reponse(int code_retour)
{
	char message[TAILLE_REQ];

	sprintf(message, "return/%d/;", code_retour);

	Emission(message);
}
