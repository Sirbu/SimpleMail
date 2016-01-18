#ifndef __SERVEUR_H__
#define __SERVEUR_H__

/** Définition des codes de retour **/
#define AUTH_ERROR  1
#define DEST_ERROR  2
#define DEL_ERROR   3
#define READ_ERROR  4
#define NO_PB       5
#define SERV_ERROR  6


#define TAILLE_REQ  4096
#define TAILLE_TYPE 512

// doit être long car le mot de passe reçu est un hash SHA512
#define TAILLE_PASS     500
#define TAILLE_LOGIN    20

// taille du tableau de lecture d'une ligne
// du fichier d'authentification
#define LINE_LENGTH     200


// #define TAILE_BUFFER    4096

/* Initialisation.
 * Creation du serveur.
 * renvoie 1 si �a c'est bien pass� 0 sinon
 */
int Initialisation();

/* Initialisation.
 * Creation du serveur en pr�cisant le service ou num�ro de port.
 * renvoie 1 si �a c'est bien pass� 0 sinon
 */
int InitialisationAvecService(char *service);


/* Attends qu'un client se connecte.
 * renvoie 1 si �a c'est bien pass� 0 sinon
 */
int AttenteClient();

/* Recoit un message envoye par le client.
 * retourne le message ou NULL en cas d'erreur.
 * Note : il faut liberer la memoire apres traitement.
 */
char *Reception();

/* Envoie un message au client.
 * Attention, le message doit etre termine par \n
 * renvoie 1 si �a c'est bien pass� 0 sinon
 */
int Emission(char *message);

/* Recoit des donnees envoyees par le client.
 * renvoie le nombre d'octets re�us, 0 si la connexion est ferm�e,
 * un nombre n�gatif en cas d'erreur
 */
int ReceptionBinaire(char *donnees, size_t tailleMax);

/* Envoie des donn�es au client en pr�cisant leur taille.
 * renvoie le nombre d'octets envoy�s, 0 si la connexion est ferm�e,
 * un nombre n�gatif en cas d'erreur
 */
int EmissionBinaire(char *donnees, size_t taille);


/* Ferme la connexion avec le client.
 */
void TerminaisonClient();

/* Arrete le serveur.
 */
void Terminaison();

/*************************************
 * En-têtes des fonctions des élèves *
 ************************************/
// extrait le type de la requête
int parseType(char* requete, char* type_requete);

// extrait le mot de passe et le login
int parseLoginPass(char* requete, char* login, char* password);

// vérifie les informations d'authentification
int checkAuthentification(char* login, char* password);

// envoie une réponse avec l'un des codes de retour
void envoi_reponse(int code_retour);


#endif
