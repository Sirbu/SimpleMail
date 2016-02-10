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

/*****************************
 * Et voici nos fonctions !  *
 *****************************/

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

/************************************
 * Cette fonction pourrait être bien plus élégante qu'elle ne l'est.
 * En utilisant une sous fonction ou des tableaux dynamiques pour
 * les paramètres par exemple. Poour l'instant je choisi la simplicitié
 ************************************/
// Parse une requête d'envoi de message.
// Extrait la source, le destinataire, l'objet
// et le contenu message.
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

// Permeet l'inscription d'un nouvel utilisateur
// dont les login et password sont dans la requête
int inscription(char* requete)
{
	char login[TAILLE_LOGIN];
	char password[TAILLE_PASS];

	char line[LINE_LENGTH];

	FILE* auth_file = NULL;

	if(parseLoginPass(requete, login, password))
	{
		fprintf(stderr, "[-] Erreur : Extraction des informations d'authentification impossible !\n");

		envoi_reponse(AUTH_ERROR);

		return 0;
	}

	auth_file = fopen("bdd", "a");
	if(auth_file == NULL)
	{
		fprintf(stderr, "[-] Erreur : Carnet d'adresses inaccessible !\n");
		envoi_reponse(SERV_ERROR);

		return SERV_ERROR;
	}

	strncpy(line, login, LINE_LENGTH - 1);
	strncat(line, ":", 1);
	strncat(line, password, LINE_LENGTH - strlen(login));
	strncat(line, "\n", 1);

	if(fwrite(line, strlen(line), 1, auth_file) != 1)
	{
		fprintf(stderr, "[-] Erreur : Écriture dans la carnet impossible !\n");
		envoi_reponse(SERV_ERROR);

		fclose(auth_file);
		return SERV_ERROR;
	}

	printf("[+] Inscription validée pour %s !\n", login);
	fclose(auth_file);

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
	// indique si on a trouvé la ligne
	// correspondant au login dans le fichier bdd
	// 0 pour non
	// 1 pour oui
	int found = 1;
	int i = 0;

	// permettra de stocker une ligne du fichier
	char line[LINE_LENGTH];

	FILE* auth_file = NULL;

	// pointeur placé à la jonction dans la ligne
	// entre login et password
	char* p_pass = NULL;

	if(checkLogin(login) != 1)
	{
		return AUTH_ERROR;
	}

	auth_file = fopen("bdd", "r");
	if(auth_file == NULL)
	{
		fprintf(stderr, "[-] FATAL ERROR : carnet d'adressses introuvable\n");
		envoi_reponse(SERV_ERROR);
		return SERV_ERROR;
	}

	do
	{
		i = 0;
		// au départ, on estime que le login est bien
		// sur cette ligne.
		found = 1;
		fgets(line, LINE_LENGTH, auth_file);
		// permet d'enlever le newline qui fait chier
		line[strlen(line)-1] = '\0';

		// on cherche le login pour savoir si c'est
		// la bonne ligne
		while(line[i] != ':' && login[i] != '\0')
		{
			if(login[i] != line[i])
			{
				found = 0;
			}
			i++;
		}

	}while(!found);


	p_pass = strchr(line, ':');
	if(p_pass == NULL)
	{
		fprintf(stderr, "[-] Erreur : Fichier d'authentification corrompu !\n");
		return 1;
	}

	// on retourne le réultat de la comparaison
	// du hash reçu avec celui dans le fichier
	p_pass++;
	// printf("[D] p_pass = %s\npassword = %s\n", p_pass, password);
	return strncmp(p_pass, password, strlen(p_pass));
}

// Gère toute la vérification de la demande
// de connexion. Les login et passwords sont
// parsés depuis la requête et stockés dans leurs tableaux
int authentification(char* requete, char* login, char* password)
{
	if(parseLoginPass(requete, login, password))
	{
		fprintf(stderr, "[-] Erreur : Extraction des informations d'authentification impossible !\n");

		envoi_reponse(AUTH_ERROR);

		return 0;
	}

	if(checkCredentials(login, password) == 0)
	{
		printf("[+] Authentification validée !\n");
		printf("[+] Bienvenue %s !\n", login);

		envoi_reponse(NO_PB);
		// est authentifié
		return 1;
	}
	else
	{
		printf("[+] Authentification refusée !\n");
		printf("[+] Dégage %s !\n", login);
		// n'est pas authentifié
		envoi_reponse(AUTH_ERROR);

		return 0;
	}
}

// Prend en paramètre la requête demandant
// l'envoi d'un message, et va stocker le dit
// message dans la boite mail du destinataire
// Retourne un code d'erreur si problème, sinon 0
int sendMessage(char* requete)
{
	// chaine contenant le nom du fichier message
	char filename[TAILLE_FILENAME];

	// variables permettant de stocker la date de l'envoi.
	// La date sera utilisée pour rendre unique le nom d'un
	// fichier
	time_t current_time;
	struct tm* c_time_struct = NULL;

	Message* mail = createMessage();

	mail->lu = '0';

	parseMessage(requete, mail);

	afficherMessage(mail);

	// vérification de l'existence du destinataire
	if(checkLogin(mail->dest) == 0)
	{
		envoi_reponse(DEST_ERROR);
		fprintf(stderr, "[-] Erreur : utilisateur %s introuvable !\n", mail->dest);
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

	strncpy(filename, mail->dest, TAILLE_FILENAME);
	filename[strlen(filename)] = '/';
	strcat(filename, mail->src);
	// écriture de la date dans le nom du fichier
	sprintf(filename+strlen(filename), "_%d-%d-%d_%d-%d-%d",
		c_time_struct->tm_mday,	c_time_struct->tm_mon, c_time_struct->tm_year,
		c_time_struct->tm_hour,	c_time_struct->tm_min, c_time_struct->tm_sec);

	// printf("[D] filename = %s\n", filename);

	if(ecrireMessage(mail, filename) == 0)
	{
		envoi_reponse(NO_PB);
	}
	else
	{
		envoi_reponse(SERV_ERROR);
		fprintf(stderr, "[-] Erreur : écriture impossible dans %s\n", filename);
		fprintf(stderr, "[-] Envoi du message impossible !\n");
		return SERV_ERROR;
	}

	return 0;
}

// retoune un code d'erreur si problème
// 0 si le login n'existe pas
// 1 si il existe
int checkLogin(char* login)
{
	// ligne lue dans le fichier d'authentification
	char line[LINE_LENGTH];

	// va contenir le login présent dans le fichier
	char credential[TAILLE_LOGIN];

	// permet de se positionner sur line
	char* p_line = NULL;

	// 0 si le login n'existe pas
	// et 1 si il existe
	int existe = 0;

	FILE* auth_file = fopen("bdd", "r");
	if(auth_file == NULL)
	{
		fprintf(stderr, "[-] Erreur : Carnet d'addresses inaccessible !\n");
		envoi_reponse(SERV_ERROR);
		return SERV_ERROR;
	}

	// tant qu'on est pas à la fin du fichier ou que existe = 1
	while(fgets(line, LINE_LENGTH, auth_file) != NULL && existe == 0)
	{
		bzero(credential, TAILLE_LOGIN);
		p_line = strchr(line, ':');

		if(p_line == NULL)
		{
			fprintf(stderr, "[-] Erreur : carnet d'adresses corrompu !\n");
			envoi_reponse(SERV_ERROR);
			fclose(auth_file);
			return SERV_ERROR;
		}

		// ici je calcule la taille en soustrayant l'adresse
		// juste après le dernier caractère du login et celle
		// du début de la ligne.
		strncpy(credential, line, (int)(p_line-line));

		if(!strncmp(credential, login, TAILLE_LOGIN))
		{
			existe = 1;
		}
	}

	fclose(auth_file);

	// printf("[D] login = %s\n[D] cred  = %s\n[D] existe = %d\n", login, credential, existe);

	return existe;
}

// compte le nombre de messages non lus
// et envoie le résultat au client
int checkNewMessages(char* login)
{
	int nbr_mess = 0;

	char reponse[TAILLE_REQ];

	// nom des fichiers messages à ouvrir
	// Attention, les fichiers sont dans le
	// dossier portant le nom du login
	char filename[100];

	// représente l'élément courant dans le dossier
	struct dirent* fichier_courant = NULL;

	Message* mail_courant = createMessage();

	DIR* boite_mail = opendir(login);
	if (boite_mail == NULL)
	{
		printf("[-] Warning : Boite mail inaccessible !\n");
		// si il n'y a pas de boite mail, l'utilisateur
		// n'a pas encore reçu de messages.
		// ATTENTION: l'erreur pourrait peut être
		// signifier autre chose...
		Emission("check/0/;");
		return 0;
	}

	while((fichier_courant = readdir(boite_mail)) != NULL)
	{
		sprintf(filename, "%s/%s", login, fichier_courant->d_name);
		// printf("[D] CHECK filename = %s\n", filename);

		// si on est pas sur le dossier courant ou parent...
		if(strchr(filename, '.') == NULL)
		{
			if(lireMessage(mail_courant, filename) != 0)
			{
				return SERV_ERROR;
			}

			// printf("[D] LU : %c\n", mail_courant->lu);

			if((char)mail_courant->lu == '0')
			{
				printf("[+] Messages non lus : %d\r", ++nbr_mess);
			}
		}
	}

	sprintf(reponse, "check/%d/;", nbr_mess);
	Emission(reponse);

	closedir(boite_mail);

	return 0;
}

// Envoie au client les en têtes des
// messages (tous ou seulement non lus)
// avec une requête par message, plus une
// requête informative au début, permettant
// au client de savoir combien de requêtes de
// réponse li doit attendre
/******************************************
 * Pourrait être opti enne faisant qu'une *
 * seule boucle... À l'aide d'un tableau  *
 * de messages...						  *
 ******************************************/
int listMessages(char* requete, char* login)
{
	// contiendra le type de la demande
	// de listing. Soit all soit new.
	char param[5];

	// requête de réponse
	char reponse[TAILLE_REQ];

	// pointeur permettant de se balader sur la requete
	char* p_requete = NULL;

	// représente le dossier
	DIR* boite_mail = NULL;

	// conient le nom complet du fichier mail
	// boite_mail/fichier_mess
	char filename[100];

	// représente l'élément courant du dossier
	struct dirent* fichier_mess = NULL;

	int i = 0;		        		  // compteur de boucle
	int j = 0;					     // autre compteur de boucle
	int nbr_new_messages = 0;		// nombre de nouveaux messages
	int nbr_messages = 0;		   // nombre de messages au total
	int nbr_requetes = 0;		  // nombre de requêtes qu'il faudra envoyer

	// Tableau de messages !
	// Contient le détail des messages à lister
	Message mails[NBR_MESS_MAX];

	bzero(mails, NBR_MESS_MAX*sizeof(Message));

	// on se place juste avant le
	// paramètre de la requête
	p_requete = strstr(requete, "/");
	if(p_requete == NULL)
	{
		fprintf(stderr, "[-] Erreur : Paramètre manquant !\n");
		envoi_reponse(SERV_ERROR);
		return SERV_ERROR;
	}
	// on enregistre le paramètre
	p_requete++;
	while(p_requete[i] != '/' && i < 5)
	{
		param[i] = p_requete[i];
		i++;
	}
	param[i] = '\0';

	// printf("[D] PARAM : %s\n", param);

	boite_mail = opendir(login);
	if(boite_mail == NULL)
	{
		printf("[-] Warning : Boite mail introuvable\n[-] Aucun message\n");
		strncpy(reponse, "info/0/;", TAILLE_REQ);
		Emission(reponse);
		return 0;
	}

	// boucle de parcours des messages
	i = 0;
	while((fichier_mess = readdir(boite_mail)) != NULL)
	{
		// on ne traite pas le dossier courant et le dossier parent
		// peut être faudra t-il tester si ce sont bien des fichiers...
		if(strchr(fichier_mess->d_name, '.') == NULL)
		{
			// arf, pas de vérif sur la taille ?!
			// Buffer overflow en vue !
			sprintf(filename, "%s/%s", login, fichier_mess->d_name);

			// printf("[D] LIST filename = %s\n", filename);
			lireMessage(mails+i, filename);

			// si on veut compter le nombre de messages non lus
			// et
			// que le message que l'on vient de lire n'a pas été lu
			if((strncmp(param, "new", 3) == 0) && ((char)mails[i].lu == '0'))
			{
				// printf("[D] new mess detected\n");
				nbr_new_messages++;
			}

			nbr_messages++;
		}

		i++;
	}

	printf("[+] Préparation des informations des %d messages\n", nbr_messages);

	// on doit déterminer le nombre de requête
	// à envoyer.
	if(strncmp(param, "new", 3))
	{
		nbr_requetes = nbr_messages;
	}
	else
	{
		nbr_requetes = nbr_new_messages;
	}

	sprintf(reponse, "info/%d/;", nbr_requetes);

	// Émission de la première trame d'informations
	// On prévient le client du nombre de requêtes
	// à venir
	printf("[D] 1ere REPONSE : %s\n", reponse);
	Emission(reponse);

	// Pour une raison que j'ignore, le tableau des
	// messages n'a qu'une case sur deux de remplies...
	// il faut donc vérifier à chaque fois les valeurs
	// d'un des champs (lu par exemple)
	i = 0;
	while(j < nbr_requetes || i < nbr_messages)
	{
		bzero(reponse, TAILLE_REQ); // probablement superflu

		afficherMessage(mails+i);

		if(strncmp(param, "new", 3) == 0)
		{
			printf("[D] Traitement mess non lus\n");
			if((char)mails[i].lu == '0')
			{
				sprintf(reponse, "info/");
				strncat(reponse, &mails[i].lu, 1);
				strncat(reponse, "/", 1);
				strncat(reponse, mails[i].src, TAILLE_LOGIN);
				strncat(reponse, "/", 1);
				strncat(reponse, mails[i].obj, TAILLE_OBJ);
				strncat(reponse, "/;", 2);

				printf("[D] INFOS reponse = %s\n", reponse);

				Emission(reponse);
				j++;
			}
		}
		else
		{
			printf("[D] Traitement messages lus\n");

			if((char)mails[i].lu == '0' || (char)mails[i].lu == '1')
			{
				sprintf(reponse, "info/");
				strncat(reponse, &mails[i].lu, 1);
				strncat(reponse, "/", 1);
				strncat(reponse, mails[i].src, TAILLE_LOGIN);
				strncat(reponse, "/", 1);
				strncat(reponse, mails[i].obj, TAILLE_OBJ);
				strncat(reponse, "/;", 2);

				printf("[D] INFOS reponse = %s\n", reponse);
				Emission(reponse);
				j++;
			}
		}

		i++;
	}

	closedir(boite_mail);

	return 0;

}

// retourne 0 si tout va bien. Un code d'erreur
// sinon. Envoie au client le message voulut,
// ainsi que l'expéditeur et l'objet.
int readMessage(char* requete, char* login)
{
	// représente le numéro du message
	int num_mess = 0;

	// bah c'est un i quoi...
	int i = 0;

	// réponse du serveur
	char reponse[TAILLE_REQ];

	// représente le dossier
	DIR* boite_mail = NULL;

	// conient le nom complet du fichier mail
	// boite_mail/fichier_mess
	char filename[100];

	// représente l'élément courant du dossier
	struct dirent* fichier_mess = NULL;

	Message* mail = createMessage();

	// extraction des paramètres...
	// je sais c'est laborieux, faut que je refactorise
	// ça dans une fonction
	if(sscanf(requete, "read/%d/;", &num_mess) != 1)
	{
		fprintf(stderr, "[-] Erreur : Numéro du message manquant !\n");
		envoi_reponse(READ_ERROR);
	}

	boite_mail = opendir(login);
	if(boite_mail == NULL)
	{
		printf("[-] Erreur : Boite mail introuvable\n[-] Aucun message\n");
		envoi_reponse(READ_ERROR);
		return READ_ERROR;
	}

	i = 1;
	while((i <= num_mess) && ((fichier_mess = readdir(boite_mail)) != NULL) )
	{
		// on ne doit pas prendre en compte
		// les dossiers '.' et '..'
		if(strchr(fichier_mess->d_name, '.') == NULL)
		{
			sprintf(filename, "%s/%s", login, fichier_mess->d_name);
			printf("[D] Lecture prochain message : %s\n", filename);

			if(lireMessage(mail, filename) != 0)
			{
				fprintf(stderr, "[-] Erreur : lireMessage(%s)\n", filename);
				closedir(boite_mail);
				return SERV_ERROR;
			}

			printf("[+] Affichage du message à envoyer :\n");
			afficherMessage(mail);

			i++;
		}
	}

	// cela veut dire que l'on a pas trouvé le message
	// correspondant au numéro demandé.
	if(fichier_mess == NULL)
	{
		fprintf(stderr, "[-] Erreur : Message n°%d introuvable !\n", num_mess);
		envoi_reponse(READ_ERROR);
		closedir(boite_mail);
		return READ_ERROR;
	}
	else
	{
		// on marque le message comme lu.
		mail->lu = '1';
		if(ecrireMessage(mail, filename) != 0)
		{
			envoi_reponse(SERV_ERROR);
			closedir(boite_mail);
		}
	}

	sprintf(reponse, "message/%s/%s/%s/;\n", mail->src, mail->obj, mail->mess);

	printf("[D] Reponse %s\n", reponse);

	Emission(reponse);

	closedir(boite_mail);

	return 0;
}


int deleteMessage(char* requete, char* login)
{
	// représente le numéro du message
	int num_mess = 0;

	// bah c'est un i quoi...
	int i = 0;

	// représente le dossier
	DIR* boite_mail = NULL;

	// contient le nom complet du fichier mail
	// boite_mail/fichier_mess
	char filename[100];

	// représente l'élément courant du dossier
	struct dirent* fichier_mess = NULL;

	if(sscanf(requete, "delete/%d/;", &num_mess) != 1)
	{
		fprintf(stderr, "[-] Erreur : Aucun numéro donné !\n");
		envoi_reponse(DEL_ERROR);
		return DEL_ERROR;
	}

	boite_mail = opendir(login);
	if(boite_mail == NULL)
	{
		printf("[-] Erreur : Boite mail introuvable\n[-] Aucun message\n");
		envoi_reponse(READ_ERROR);
		return READ_ERROR;
	}

	i = 1;
	while((i <= num_mess) && ((fichier_mess = readdir(boite_mail)) != NULL))
	{
		if(strchr(fichier_mess->d_name, '.') == NULL)
		{
			printf("[D] Message %d\n", i);
			i++;
		}
	}

	sprintf(filename, "%s/%s", login, fichier_mess->d_name);

	if(remove(filename) != 0)
	{
		perror("[-] Erreur : ");
		envoi_reponse(SERV_ERROR);
		return SERV_ERROR;
	}

	printf("[+] Message supprimé\n");
	envoi_reponse(NO_PB);

	return 0;
}


// envoie une réponse contenant le code donné
// en paramètre
void envoi_reponse(int code_retour)
{
	char message[TAILLE_REQ];

	sprintf(message, "return/%d/;", code_retour);
	printf("trame envoyée : %s\n", message);

	Emission(message);
}

/*****************************************************
 * Fonctions ayant rapport avec la structure Message *
 *****************************************************/

 // retourne un pointeur pointant
 // sur une zone allouée ayant la taille
 // d'un message
 Message* createMessage()
 {
	Message* m = malloc(sizeof(Message));
	if(m == NULL)
	{
		fprintf(stderr, "[-] Erreur : Malloc  message impossible !\n");
		exit(EXIT_FAILURE);
	}

    return m;
 }

// Lit le message contenu dans le fichier donné en paramètre.
// et le stocke dans la structure Message pointée par mail
// Retourne un code d'erreur si il y en a une. 0 sinon
// la fonction est pas très jolie(comme pas mal d'autres endroits
// dans le code...), mais je refactoriserai
// le code dans sa globalité plus tard. Là on a pas des masses
// de temps.
int lireMessage(Message* mail, char* fichier)
{
	FILE* fic = fopen(fichier, "r");
	if(fic == NULL)
	{
		printf("[-] Erreur : fichier %s introuvable !", fichier);
		envoi_reponse(SERV_ERROR);
		return(SERV_ERROR);
	}

	mail->lu = (char)fgetc(fic);

	fgetc(fic);

	if(fgets(mail->src, TAILLE_LOGIN, fic) == NULL)
	{
		fprintf(stderr, "[D] Erreur : lecture impossible -> %s\n", fichier);
		return SERV_ERROR;
	}
	mail->src[strlen(mail->src)-1] = '\0';

	if(fgets(mail->dest, TAILLE_LOGIN, fic) == NULL)
	{
		fprintf(stderr, "[D] Erreur : lecture impossible -> %s\n", fichier);
		return SERV_ERROR;
	}
	mail->dest[strlen(mail->dest)-1] = '\0';

	if(fgets(mail->obj, TAILLE_OBJ, fic) == NULL)
	{
		fprintf(stderr, "[D] Erreur : lecture impossible -> %s\n", fichier);
		return SERV_ERROR;
	}
	mail->obj[strlen(mail->obj)-1] = '\0';

	if(fgets(mail->mess, TAILLE_MESS, fic) == NULL)
	{
		fprintf(stderr, "[D] Erreur : lecture impossible -> %s\n", fichier);
		return SERV_ERROR;
	}
	mail->mess[strlen(mail->mess)-1] = '\0';

	fclose(fic);

	return 0;
}

// retourne un code d'erreur si il y a un problème
// retourne 0 si tout va bien
int ecrireMessage(Message* mail, char* fichier)
{
	FILE* fic = fopen(fichier, "w");
	if(fic == NULL)
	{
		printf("[-] Erreur : fichier mail %s introuvable !\n", fichier);
		envoi_reponse(SERV_ERROR);
		return(SERV_ERROR);
	}

	fprintf(fic, "%c\n%s\n%s\n%s\n%s\n",
		(char)mail->lu, mail->src, mail->dest, mail->obj, mail->mess);

	fclose(fic);

	return 0;
}

void afficherMessage(Message* mail)
{
	printf("[+] Affichage des informations du message :\n");
	printf(" -> Lu : %c\n", mail->lu);
	printf(" -> Source : %s\n", mail->src);
	printf(" -> Destinataire : %s\n", mail->dest);
	printf(" -> Objet : %s\n", mail->obj);
	printf(" -> Contenu : %s\n", mail->mess);
}
