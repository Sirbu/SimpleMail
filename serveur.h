#ifndef __SERVEUR_H__
#define __SERVEUR_H__


/** Définition des codes de retour **/
#define AUTH_ERROR  1
#define DEST_ERROR  2
#define DEL_ERROR   3
#define READ_ERROR  4
#define NO_PB       5
#define SERV_ERROR  6


#define TAILLE_REQ      4096        // Taille max d'une requête

#define TAILLE_MESS     3500        // Taille max d'un message (le contenu)
#define TAILLE_TYPE     512         // Taille max du type de la requête
#define TAILLE_OBJ      50          // Taille max du l'objet du message

// nombre maximal de messages dans
// la boite mail
#define NBR_MESS_MAX    100

// doit être long car le mot de passe reçu est un hash SHA512
#define TAILLE_PASS         500
#define TAILLE_LOGIN        20

#define TAILLE_FILENAME     50      // taille max du nom d'un fichier message

// taille du tableau de lecture d'une ligne
// du fichier d'authentification
#define LINE_LENGTH     200

// structure décrivant un message
typedef struct mess
{
    // lu est un char car cela simplifie la lecture
    // de sa valeur dans le fichier
    char lu;                         // 1 si lu, 0 sinon

    char src[TAILLE_LOGIN];        // Expéditeur du message
    char dest[TAILLE_LOGIN];      // Destinataire du message
    char obj[TAILLE_OBJ];        // Objet du message
    char mess[TAILLE_MESS];     // Contenu du message

}Message;

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
int checkCredentials(char* login, char* password);

// envoie une réponse avec l'un des codes de retour
void envoi_reponse(int code_retour);

// Gère la totalité de l'authentification
int authentification(char* requete, char* login, char* password);

// Parse une requête d'envoi de message.
// Extrait source, destinataire, objet, contenu, message
// retourne un message d'erreur au client si cela se passe mal
// renvoie 1 si il y a une erreur, 0 sinon
int parseMessage(char* requete, Message* mail);

// gère la réception de messages
int sendMessage(char* requete);

// vérifie l'exsitence du login donné
// en paramètre
int checkLogin(char* login);

// retourne le nombre de messages non lus
// et envoie la même valeur au client
int checkNewMessages(char* login);

// Envoie au client les en têtes des
// messages (tous ou seulement non lus)
// avec une requête par message, plus une
// requête informative au début, permettant
// au client de savoir combien de requêtes de
// réponse li doit attendre
int listMessages(char* requete, char* login);

// readMessage envoie le contenu du message
// désigné par un numéro dans la requête.
int readMessage(char* requete, char* login);

// supprime le message désigné par un
// numéro dans la requête.
int deleteMessage(char* requete, char* login);

// Inscrit un nouvel utilisateur.
// La requête contient le login et le mot de passe à écrire.
// Si l'utilisateur est déjà inscrit, la demande est rejetée.
int inscription(char* requete);

/********************
 * Fonctions Message
 *******************/
Message* createMessage();

int lireMessage(Message* mail, char* fichier);
int ecrireMessage(Message* mail, char* fichier);

void afficherMessage(Message* mail);


#endif
