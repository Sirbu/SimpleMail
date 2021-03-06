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


/*authentification

*/
int authentification(char *login){
	int code_ret;
	char request[TAILLE_REQUETTE];
	char password[TAILLE_PASSWORD];

	/********************************************************/
	couleur("46");


	printf("						VEUILLEZ PROCEDER A VOTRE AUTHENTIFICATION \n");
	printf("					LOGIN: ");
	fgets(login,TAILLE_ID,stdin);
	login[strlen(login)-1]='\0';/*elimination du retour a la ligne*/
	printf("					PASSWORD: ");
	fgets(password,TAILLE_PASSWORD,stdin);
	password[strlen(password)-1]='\0';/*elimination du retour a la ligne*/
	couleur("0");
	/*cryptage du mot de passe avant l'envoie*/
	strncpy(password,crypt(password,"$6$"),TAILLE_PASSWORD);

	/*formulation de la requette*/

	sprintf(request,"authentification/%s/%s/;",login,password);

	/*envoie de la requette*/

	Emission(request);

	/*attente de la reponse*/
	char *response=Reception();

	if (response==NULL || sscanf(response,"return/%d/;",&code_ret) != 1 ){
		printf("une erreur s'est produite!!\n");

		free(response);//liberation de la memoire allouee par strdup
		Terminaison();// fermeture de la connexion

		exit(INTERN_ERROR);

    }
	else
		//printf("[D] REPONSE = %s && code_ret = %d\n", response, code_ret);// DEBBUG
		return (code_ret);

}

/*ce sous programme prend en paramettre @ de l'expediteur
 recuperera les champs necessaire pour l'envoie d'un message
*/
void Envoyermessage(char login[]){

	char continuer='o';
	char dest[TAILLE_ID];
	char objet[TAILLE_PASSWORD];
	char contenu[TAILLE_CONETENU];
	char request[TAILLE_REQUETTE];

	int code_ret;


	printf("@ DESTINATRICE : ");
	fgets(dest,TAILLE_ID,stdin);
	dest[strlen(dest)-1]='\0';

	printf("OBJET : ");
	fgets(objet,TAILLE_PASSWORD,stdin);//etant donnée qu'on avait defini une taille assez grande pour le password je l'ai reutiliser ici
	objet[strlen(objet)-1]='\0';// elimination de retour a la ligne
	transparence(objet);// nous permet d'elimner les '/' et les remplacer par '\'
						// si jamais il existe
	// rajout du mot (recu) qui sera utilie au serveur pour rajouter la date de reception
	strcat(objet ," recu:");
	printf("CONTENU : ");
	fgets(contenu,TAILLE_CONETENU,stdin);
	contenu[strlen(contenu)-1]='\0';

	/*formulation de la requette*/
	sprintf(request,"send/%s/%s/%s/%s/;",login,dest,objet,contenu);
    /*envoie de la requette*/

	Emission(request);

	/*reception de la requette*/

	char* response=Reception();

	if (response == NULL || sscanf(response,"return/%d/;",&code_ret)!=1 || code_ret == SERV_ERROR){
		printf("une erreur s'est produite");
		Terminaison();
		exit(INTERN_ERROR);

	}

	else if (code_ret == NO_PB){
		couleur("32");
		printf("message envoyé avec succés !!\n");
		couleur("0");
}
	else{
			printf("vous vous etes tropmé de destinataire voulez vous reessayer? o/oui n/non ");
			continuer=fgetc(stdin);
			viderBuffer();
			while(continuer=='o' && code_ret == DEST_ERROR ){

				printf("  RESSAISI @ DESTINATRICE : ");

				fgets(dest,TAILLE_ID,stdin);
				dest[strlen(dest)-1]='\0';

				bzero(request,TAILLE_REQUETTE);//on vide la chaine de charactere

				/*formulation de la requette*/
				sprintf(request,"send/%s/%s/%s/%s/;",login,dest,objet,contenu);

				Emission(request);

				response=Reception();

				if (response ==NULL || sscanf(response,"return/%d/;",&code_ret)!=1 ){
					printf("une erreur s'est produite\n");
					Terminaison();
					exit(INTERN_ERROR);
				}
				if(code_ret== DEST_ERROR ){
					printf("vous vous etes tropmé de destinataire voulez vous reessayer? o/oui n/non ");
					continuer=fgetc(stdin);
					viderBuffer();
				}
				else if(code_ret != NO_PB){
					printf("une erreur s'est produite!! %d \n",code_ret);
					Terminaison();
					exit(code_ret);
				}
			}

		if (code_ret== NO_PB){
			couleur("32");
			printf("MESSAGE ENVOYE AVEC SUCCES \n");
			couleur("0");
		}

	}// une fois ici c'est que tout s'est bien passé(reussi a envoyer ou abandonne )
	free (response);
}

/*deconnexion
*/
void deconnexion(){

	Emission("disconnect/;");
	printf("***************************\n vous ete maintenant deconnécté ,à bientot\n***************************\n");

}

/*le sous programme se chargera
 *d'envoyer une requette au serveur pour savoir si
 * il y'a eu des nouveaux messages ou pas
*/

void check(){

	int code_ret;

	Emission("check/;");//envoie de la requette

	char *response = Reception();

	if (response == NULL || sscanf(response,"check/%d/;",&code_ret)!= 1){// teste de la requette
				printf("une erreur intern s'est produite");
				Terminaison();
				exit(INTERN_ERROR);

    }
	couleur("41");
	printf("		VOUS AVEZ %d NOUVEAUX MESSAGES			\n",code_ret);
	couleur("0");
	free(response);
}
/*affiche le menu d'entrée
*/
void afficher_menu(){
	couleur("46");
	printf("		*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
	printf("			BIENVENUE SUR MEDISCU MESSENGER \n");
	printf("		#		  1 : vous connecter    \n");
	printf("		#	_______________________________	\n");
	printf("		#		  2 : vous inscrire	\n");
	printf("		#	_______________________________	\n");
	printf("		#		  3 : quitter		\n");
	printf("		#	________________________________\n");
	printf("		*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+\n");
	printf("			en attente de votre choix:      \n");
	couleur("0");

}
/*affiche le menu principale
*/
void afficher_menu1(){
	couleur("46");
	printf("*******************************************\n");
	printf("		a : deconnexion	           \n");
	printf("#	_______________________________    \n");
	printf("		b : envoyer un message	   \n");
	printf("	_______________________________    \n");
	printf("#		c : consulter vos messages \n");
	printf("*******************************************\n");
	printf("	en attente de votre choix:         \n");
	couleur("0");
}
/*affiche le menu secondaire
 * correspendant au choix c
*/
void afficher_menu2(){
	couleur("46");
	printf("\n***************************************************************\n");
	printf("		d : ya t-il des nouveaux message ?						#\n");
	printf("#	______________________________________						#\n");
	printf("		e : liste des nouveaux messages 						#\n");
	printf("#	______________________________________						#\n");
	printf("		f : lire un message										#\n");
	printf("#	______________________________________						#\n");
	printf("		g : liste de tout les messages 							#\n");
	printf("#	______________________________________						#\n");
	printf("		h : suppression d'un message 							#\n");
	printf("#	______________________________________						#\n");
	printf("		i : retourner au menu principal							#\n");
	printf("\n***************************************************************\n");
	printf("	en attente de votre choix: \n");
	couleur("0");
}
/*sous programme qui se chargera de l'affichage
 * des en-tetes des  messages(nouveaux/tout)
 elle renverra aussi le nombre de message
*/
int list(char *param){
	char request[TAILLE_REQUETTE];
	int nbre ;
	int j= 0;
	int pos=0;
	char expediteur[TAILLE_ID];
	char objet[TAILLE_PASSWORD];
	char etat;
	char *paid;
	sprintf(request,"list/%s/;",param);//formulation de la requette
	Emission(request);

	char* response = Reception();// reception de la premiere requette qui nous indiquera le nombre de message
								//pour pouvoir se synchroniser a l'aide d'une boucle

	if (response == NULL || sscanf(response,"info/%d/;",&nbre)!= 1){// test de la requette
		printf("une erreur s'est produite\n");
		Terminaison();
		exit(INTERN_ERROR);
	}

	if (nbre == 0){
		couleur("32");
		printf("		VOUS N'AVEZ AUCUN NOUVEAUX MESSAGES 			\n");
		couleur("0");
	}
	else{// on rentre dans une boucle pour recevoir les requette l'une a la suite des autres

		for(j = 1 ; j <= nbre ; j++){// reception,extraction puis affichage des parametres

			char *response=Reception();//recetpion

			if(response == NULL){// teste de la requette
				printf("une erreur s'est produite");
				Terminaison();
				exit(SERV_ERROR);
			}
			//printf("[DEBBUG]%s\n",response );//DEBBUG
			pos=0;//remise a zero du compteur
			paid=strchr(response,'/');// paid pointera sur le premier '/' trouvé
			paid++;// on recupere le deuxiemme champ(info/etat/expediteur/objet/;)
			etat=(*paid);
			paid=paid+2;
			// on recupere les champs en question caractere par caractere
			while((*paid) != '/'){
				expediteur[pos] =(*paid);
				pos++;
				paid++;
			}
			expediteur[pos] = '\0';//terminer la chaine correctement
			paid++;
			pos=0;

			while((*paid) != '/' && (*(paid+1)) != ';'){
				objet[pos] =(*paid);
				paid++;
				pos++;
			}
			objet[pos] = '\0';
			anti_transparence(objet);// remettre les /

			// une fois ici on aura recuperer les champs requis pour l'affichage
			if(etat == '0'){//on affichera les infos des message non lus en couleur
				couleur("41");
				printf(" NEW <%d> expediteur : %s",j,expediteur);
				couleur("0");
				printf("\n");
				couleur("41");
				printf("objet: %s",objet );
				couleur("0");
				printf("\n");

				printf("_________________________________________");
				printf("\n");

		    }
			else{// si il est deja lu on l'affiche normalement
				printf("<%d> expediteur : %s \n objet : %s\n _________________________________________\n",j,expediteur,objet);

			}
			// vider les chaine de caracteres
			bzero(expediteur,TAILLE_ID);
			bzero(objet,TAILLE_PASSWORD);
			free (response);// liberation de la memoire allouee par strdup
		}
	}
	return nbre;
}
/*ce sous programme se chargera de demander a l'utilisateur
*le message qu'il voudra lire puis le luis affichera
*/

void lire(){
	char request[TAILLE_REQUETTE];
	char expediteur[TAILLE_ID];
	char objet[TAILLE_PASSWORD];
	char contenu[TAILLE_CONETENU];
	int i=8;//on se place directement sur le debut du champ objet
	int pos=0;
	int nbre=0;
	char rep='o';

	//permettre a l'utilisateur d'avoir les message sous les yeux

	int code_ret=list("all");

	if (code_ret){// on ne lui demandera de la saisie du message que si effectivement il en a

		do{
			printf("rentrez le numero du message a lire (entre 1 et %d) :  ",code_ret);
			scanf("%d",&nbre);
			viderBuffer();
			if ( nbre > code_ret){
				printf("vous avez saisi un mauvais numero voulez vous ressaisir ? o/n " );
				rep=getchar();
				viderBuffer();
			}

		}while( rep == 'o' && (nbre > code_ret || nbre < 1) );

		if (nbre <= code_ret){

			sprintf(request,"read/%d/;",nbre);

			Emission(request);

			char* response = Reception();
			if (response == NULL){
				printf("une erreur s'est produite /n");
				Terminaison();
				exit(INTERN_ERROR);
			}
			//printf("[debbug] %s\n",response );

			while(response[i]!='/'){// recuperation du champ expediteur
				expediteur[pos]=response[i];
				i++;
				pos++;
			}
			expediteur[pos]='\0';
			pos=0;
			i++;
			while (response[i]!='/'){//recuperation de l'objet
				objet[pos]=response[i];
				i++;
				pos++;
			}
			objet[pos]='\0';
			anti_transparence(objet);
			pos=0;
			i++;
			while(response[i]!='\0' && response[i+1]!=';'){//etant donnée que la requette se termine par /;
				contenu[pos]=response[i];
				i++;
				pos++;
			}
			contenu[pos]='\0';
			printf("________________________________________________\nexpediteur: %s\nobjet: %s\ncontenu: %s\n________________________________________________\n",expediteur,objet,contenu);

			free(response);
		}
	}
}

/*ce sous programme se chargera
*de la suppression d'un message
*/
void supprimer(){
	char request[TAILLE_REQUETTE];
	int num;
	int nbre_message=list("all");
	char rep='o';

	if( nbre_message ){// il ne supprimera des message que si l'on a au moins 1

		do{
			printf("rentrez le numero du message a supprimer (entre 1 et %d) :  ",nbre_message);
			scanf("%d",&num);
			viderBuffer();
			if ( num > nbre_message){
				printf("vous avez saisi un mauvais numero voulez vous ressaisir ? o/n " );
				rep=getchar();
				viderBuffer();
			}
		}while(rep=='o' && ( num > nbre_message || num < 1));

		if ( num <= nbre_message){
			printf("etes vous sur de vouloir supprimer ?? o/n ");
			rep=getchar();
			viderBuffer();
			if( rep == 'o'){
				sprintf(request,"delete/%d/;",num);
				Emission(request);

				char *response=Reception();

				if( response == NULL || sscanf(response,"return/%d/;",&num) != 1 ){
					printf("une erreur s'est produite [delete] \n" );
					Terminaison();
					exit(INTERN_ERROR);
				}
				if(num== NO_PB){
					couleur("32");
					printf("MESSAGE SUPPRIME AVEC SUCCES					  ");
					couleur("0");
				}
				else{
					couleur("32");
					printf("LE MESSAGE N'A PAS PU ETRE SUPPRIMER			  ");
					couleur("0");
				}
				free(response);
			}


		}


	}


}
/*ce sous programme se charge
 *de d'inscrire l'utilisateur
 */
 void inscription(){
	int num;
	char login[TAILLE_ID];
	char request[TAILLE_REQUETTE];
	char password[TAILLE_PASSWORD];
	char password_test[TAILLE_PASSWORD];
	char * response;

	printf("			rentrez le login: ");
	fgets(login,TAILLE_ID,stdin);
	do{//on demandera a l'utilisateur de saisir le mot de passe 2 fois
		// on ne valide la saisi que si les deux mot de passe sont identique
		printf("		rentrer le mot de passe:");
		fgets(password,TAILLE_PASSWORD,stdin);

		printf("		veuillez confirmer le mot de passe :");
		fgets(password_test,TAILLE_PASSWORD,stdin);
		password[strlen(password)-1]='\0';// elimination des \n
		password_test[strlen(password_test)-1]='\0';
		// controle de saisi
		if(strchr(login,'/') != NULL || strchr(login,':') != NULL){
			printf("attention le login contient des characteres interdit('/',':')\n");
			printf("veuillez ressaisir le login\n");
			fgets(login,TAILLE_ID,stdin);
		}
		if (strcmp(password_test,password)){
			couleur("46");
			printf("attention les mot de passes ne sont pas identique \n");
			couleur("");
		}
	}while(strcmp(password_test,password) || strchr(password,'/') != NULL || strchr(password,':') != NULL);

	login[strlen(login)-1]='\0';
	strncpy(password,crypt(password,"$6$"),TAILLE_PASSWORD);
	sprintf(request,"inscription/%s/%s/;",login,password);
	//printf("debbug %s ",request);

	Emission(request);

	response=Reception();

	if( response == NULL || sscanf(response,"return/%d/;",&num) != 1 ){// verification du bon deroulement de la reception
																		// et de l'extraction des parametres
		printf("une erreur s'est produite  \n" );
		Terminaison();
		exit(INTERN_ERROR);
	}

	if ( num == NO_PB){// contre du bon deroulement de l'inscription
		couleur("32");
		printf ("inscription reussi \n");
		couleur("0");
	}
	else{
		couleur("42");
		printf("l'inscrption a echoué veuillez reessayer \n");
		couleur("0");
	}
	free(response);
}
/*ce sous programme nous permettra
*d'eliminer le charactere '/' et le remplacer par un autre'\'
*pour ne pas avoir confusion entre le / et la separation des champs
*dans ma requette
*/
void transparence(char tab[]){
	int i =0;

	while ((tab[i] !='/') && (tab[i]!= '\0'))
	{
		i++;
	}
	if(tab[i]== '/')
	{
		tab[i]='\\';
	}
}
/*ce sous programme nous permet de de faire l'operation inverse
*remettre le / en recevant ma traitement
*ca fait la ceciproque de transparence
*/
void anti_transparence(char tab[]){
	int i =0;

	while ((tab[i] !='\\') && (tab[i]!= '\0'))
	{
		i++;
	}
	if(tab[i]== '\\')
	{
		tab[i]='/';
	}
//printf("%s",tab); debbug

}
