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

/**** Includes personnels ****/
#include <crypt.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>

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
	// printf("[D] User : %s length = %d\n", p_requete, (int)strlen(p_requete));
	while(p_requete[i] != '/')
	{
		login[i] = p_requete[i];
		i++;
	}
	// le petit 0 qui termine la chaine
	login[i] = '\0';

	// permet de placer le pointeur p_requete
	// sur le '/' juste avant le mot de passe
	p_requete = strchr(p_requete, '/');


	// on incrémente encore une fois pour
	// qu'il se place juste après le '/'
	p_requete++;

	strncpy(password, p_requete, TAILLE_PASS);

	// printf("[D] Password : %s\n", password);

	return 0;
}

// vérifie les informations d'authentifcation en paramètres.
// Attention, le mot de passe est un hash contenant l'id
// de l'algorithme, le sel et le mot de passe. Le sel est une
// chaine vide, et chaque champ est séparé par un '$' comme suit :
// $id$salt$hash
// Retourne 0 si tout s'est bien passé.
// 1 si le login ou le mot de passe n'est pas bon
int checkCredentials(char* login, char* password)
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
		// printf("[D] login = %s\nLine = %s\n", login, line);
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
	// printf("[D] p_pass = %s\npassword = %s\n", p_pass, password);
	return strncmp(p_pass, password, strlen(p_pass));
}

// envoie une réponse contenant le code donné
// en paramètre
void envoi_reponse(int code_retour)
{
	char message[TAILLE_REQ];

	sprintf(message, "return/%d/;", code_retour);

	// printf("[D] return request : %s\n", message);

	Emission(message);
}

// Gère toute la vérification de la demande
// de connexion
int authentification(char* requete, char* login, char* password)
{
	if(parseLoginPass(requete, login, password))
	{
		fprintf(stderr, "[-] Erreur : Extraction des informations d'authentification impossible !\n");

		envoi_reponse(AUTH_ERROR);

		exit(EXIT_FAILURE);
	}

	if(checkCredentials(login, password) == 0)
	{
		printf("[+] Authentification validée !\n");
		printf("[+] Bienvenue %s!\n", login);

		envoi_reponse(NO_PB);
		// est authentifié
		return 1;
	}
	else
	{
		printf("[+] Authentification refusée !\n");
		printf("[+] Dégage %s!\n", login);
		// n'est pas authentifié
		envoi_reponse(AUTH_ERROR);

		return 0;
	}
}


/************************************
 * Cette fonction pourrait être bien plus élégante qu'elle ne l'est.
 * En utilisant une sous fonction ou des tableaux dynamiques pour
 * les paramètres par exemple. Poour l'instant je choisi la simplicitié
 ************************************/
// Parse une requête d'envoi de message.
// Extrait source, destinataire, objet, contenu, message
// retourne un message d'erreur au client si cela se passe mal
// renvoie 1 si il y a une erreur, 0 sinon
// NB: vérifier le nombre de paramètres un peu mieux que ça.
int parseMessage(char* requete, Message* mail)
{
	// pointeur qui va se situer successivement sur les '/'
	// séparant les paramètres de la requête. Il permettra
	// de le se situer juste avant un paramètre pour pouvoir l'extraire
	char* p_requete = NULL;
	int i = 0; 	// itérateur de boucle

	// extraction de la source
	p_requete = strchr(requete, '/');
	if(p_requete == NULL)
	{
		printf("[-] Erreur : Il manque un paramètre !\n");
		envoi_reponse(DEST_ERROR);
		return 1;
	}
	p_requete++;
	while(p_requete[i] != '/')
	{
		mail->src[i] = p_requete[i];
		i++;
	}
	mail->src[i] = '\0';
	i = 0;

	// extraction du destinataire
	p_requete = strchr(p_requete, '/');
	if(p_requete == NULL)
	{
		printf("[-] Erreur : Il manque un paramètre !\n");
		envoi_reponse(DEST_ERROR);
		return 1;
	}
	p_requete++;
	while(p_requete[i] != '/')
	{
		mail->dest[i] = p_requete[i];
		i++;
	}
	mail->dest[i] = '\0';
	i = 0;

	// extraction de l'objet
	p_requete = strchr(p_requete, '/');
	if(p_requete == NULL)
	{
		printf("[-] Erreur : Il manque un paramètre !\n");
		envoi_reponse(DEST_ERROR);
		return 1;
	}
	p_requete++;
	while(p_requete[i] != '/')
	{
		mail->obj[i] = p_requete[i];
		i++;
	}
	mail->obj[i] = '\0';
	i = 0;

	// extraction du message
	p_requete = strchr(p_requete, '/');
	if(p_requete == NULL)
	{
		printf("[-] Erreur : Il manque un paramètre !\n");
		envoi_reponse(DEST_ERROR);
		return 1;
	}
	p_requete++;	// on se positionne sur le début du message
	strncpy(mail->mess, p_requete, TAILLE_MESS);

	// j'enlève le '/;' à la fin
	mail->mess[strlen(mail->mess)-2] = '\0';

	return 0;
}

// permet d'extraire un paramètre, pointé par
// p_start. Il sera retourné comme un char*
// Cette fonction ne pourra pas être utilisée
// pour extraire le contenu du message, car elle
// se base sur la présence de '/' entre les paramètres.
// Et c'est un caractère qui peut se retrouver dans le contenu
// char* parseParam(char* requete, char* p_start)
// {
// 	int i = 0;
//
// 	// chaine qui sera retournée grâce à strdup()
// 	char param[TAILLE_PARAM];
//
// 	// boucle jusqu'a ce que p_param = NULL
// 	// Mais vérification de sa nullité dans le corps
// 	// de boucle au cas ou on ne trouve pas de '/'
//
// 	while(p_requete[i] != '/')
// 	{
// 		param[i] = p_requete[i++];
// 	}
// 	param[i] = '\0';
//
// 	return strdup(param);
// }


// Prend en paramètre la requête demandant
// l'envoi d'un message, et va stocker le dit
// message dans la boite mail du destinataire
// Retourne un code d'erreur si problème, sinon 0
int sendMessage(char* requete)
{
	// identifcateur du fichier message
	FILE* fichier_mess = NULL;

	int ret = 0; 		// retour de fonction fwrite

	// chaine contenant le nom du fichier message
	char filename[TAILLE_FILENAME];

	// variables permettant de stocker la date de l'envoi.
	// La date sera utilisée pour rendre unique le nom d'un
	// fichier
	time_t current_time;
	struct tm* c_time_struct = NULL;

	Message* mail = createMessage();

	mail->lu = 0;	// ce message n'est pas encore lu

	parseMessage(requete, mail);

	afficherMessage(mail);

	// vérification de l'existence du destinataire
	if(checkDest(mail->dest) == 0)
	{
		envoi_reponse(DEST_ERROR);
		return DEST_ERROR;
	}

	// chaque boite mail (dossier) porte le nom
	// du compte associé
	// on vérifie donc si le dossier correspondant
	// au destinataire existe
	// sinon il faut le créer.
	if(access(mail->dest, F_OK) == -1)
	{
		printf("[-] Warning : Aucune boite mail détecté pour %s !\n", mail->dest);
		printf("[+] Création de la boite mail...\n");
		if(mkdir(mail->dest, 0700) == -1)
		{
			printf("[-] Erreur : impossible de créer la boite mail pour %s\n", mail->dest);
			perror(mail->dest);
			envoi_reponse(SERV_ERROR);
			return SERV_ERROR;
		}
	}

	// Le nom relatif de fichier sera composé du nom
	// de la boite mail (dossier) ainsi que du nom du
	// du fichier message (nom de l'expéditeur + date expédition)

	// générer la date d'envoi
	current_time = time(NULL);
	c_time_struct = localtime(&current_time);
	// printf("TIME : %d-%d-%d_%d-%d\n", c_time_struct->tm_mday,
	// 	c_time_struct->tm_mon, c_time_struct->tm_year, c_time_struct->tm_hour,
	// 	c_time_struct->tm_min);


	strncpy(filename, mail->dest, TAILLE_FILENAME);
	filename[strlen(filename)] = '/';
	strcat(filename, mail->src);
	// écriture de la date dans le nom du fichier
	sprintf(filename+strlen(filename), "_%d-%d-%d_%d-%d-%d", c_time_struct->tm_mday,
		c_time_struct->tm_mon, c_time_struct->tm_year, c_time_struct->tm_hour,
		c_time_struct->tm_min, c_time_struct->tm_sec);

	printf("[D] filename = %s\n", filename);

	fichier_mess = fopen(filename, "w");
	if(fichier_mess == NULL)
	{
		fprintf(stderr, "[-] Erreur : Ouverture en écriture du fichier message impossible !\n");
		envoi_reponse(SERV_ERROR);
		return SERV_ERROR;
	}

	// if((ret = fwrite(mail, sizeof(Message), 1, fichier_mess)) != 1)
	// {
	// 	fprintf(stderr, "[-] Erreur : problème lors de l'écriture !\n");
	// 	perror(filename);
	// 	envoi_reponse(SERV_ERROR);
	// 	return SERV_ERROR;
	// }

	ret = fprintf(fichier_mess, "%d\n%s\n%s\n%s\n%s\n", mail->lu, mail->src, mail->dest, mail->obj, mail->mess);

	printf("[D] Retour fprintf = %d\n", ret);

	envoi_reponse(NO_PB);

	fclose(fichier_mess);

	return 0;
}

// retoune un code d'erreur si problème
// 0 si le destinataire n'existe pas
// 1 si il existe
int checkDest(char* destinataire)
{
	// ligne lue dans le fichier d'authentification
	char line[500];

	// 0 si le destinataire n'existe pas
	// et 1 sinon
	int existe = 0;

	FILE* auth_file = fopen("bdd", "r");
	if(auth_file == NULL)
	{
		fprintf(stderr, "[-] Erreur : Carnet d'addresses inaccessible !\n");
		envoi_reponse(SERV_ERROR);
		return SERV_ERROR;
	}

	// tant qu'on est pas à la fin du fichier ou que existe
	while(fgets(line, 500, auth_file) != NULL && existe != 1)
	{
		if(strstr(line, destinataire) != NULL)
		{
			existe = 1;
		}
	}

	return existe;
}


/*********************
 * Fonctions ayant rapport avec Message
 * Je les mets ici car je n'ai pas réussi à me
 * débrouiller avec les multiples inclusions bouclées
 * que provoquaient la présence de mail.c et mail.h
 *********************/

 // retourne un pointeur pointant
 // sur une zone allouée ayant la taille
 // d'un message
 Message* createMessage()
 {
	Message* m = malloc(sizeof(Message));
	if(m == NULL)
	{
		fprintf(stderr, "[-] Erreur : Malloc  message !\n");
		exit(EXIT_FAILURE);
	}

    return m;
 }

// Lit le message contenu dans le fichier donné en paramètre.
// et le stocke dans la structure Message pointée par mail
// Retourne un code d'erreur si il y en a une. 0 sinon
int lireMessage(Message* mail, char* fichier)
{
	FILE* fic = fopen(fichier, "r");
	if(fic == NULL)
	{
		printf("[-] Erreur : fichier mail %s introuvable !\n", fichier);
		envoi_reponse(DEST_ERROR);
		return(DEST_ERROR);
	}

	while(!feof(fic))
	{
		// j'ai du mal à savoir si le choix
		// taille/nbr_elmts est judicieux
		// Je voudrais lire la totalité du fichier, dont je ne
		// connais pas la taille, et le stocker dans la structure.
		// Les données sont ordonnées comme il le faut.
		if(fread(mail, 1, 1, fic))
		{
			perror("[-] Erreur lecture");
			return SERV_ERROR;
		}
	}

	return 0;
}

int ecrireMessage(Message* mail, char* fichier)
{
	FILE* fic = fopen(fichier, "w");
	if(fic == NULL)
	{
		printf("[-] Erreur : fichier mail %s introuvable !\n", fichier);
		envoi_reponse(DEST_ERROR);
		return(DEST_ERROR);
	}

	if(fwrite(mail, sizeof(Message), 1, fic))
	{
		perror("[-] Erreur écriture");
		return SERV_ERROR;
	}

	return 0;
}

void afficherMessage(Message* mail)
{
	printf("[+] Affichage des informations du message :\n");
	printf(" -> Lu : %d\n", mail->lu);
	printf(" -> Source : %s\n", mail->src);
	printf(" -> Destinataire : %s\n", mail->dest);
	printf(" -> Objet : %s\n", mail->obj);
	printf(" -> Contenu : %s\n", mail->mess);
}
