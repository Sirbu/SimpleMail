#ifndef __CLIENT_H__
#define __CLIENT_H__
#define AUTH_ERROR 1
#define DEST_ERROR 2
#define DEL_ERROR  3
#define READ_ERROR 4
#define NO_PB	   5
#define SERV_ERROR 6
#define INTERN_ERROR 9
#define TAILLE_REQUETTE 4096
#define TAILLE_ID 20
#define TAILLE_PASSWORD 200
#define TAILLE_CONETENU 3500
/* Initialisation.
 * Connexion au serveur sur la machine donnee.
 * Utilisez localhost pour un fonctionnement local.
 * renvoie 1 si �a c'est bien pass� 0 sinon
 */
int Initialisation(char *machine);

/* Initialisation.
 * Connexion au serveur sur la machine donnee et au service donne.
 * Utilisez localhost pour un fonctionnement local.
 * renvoie 1 si �a c'est bien pass� 0 sinon
 */
int InitialisationAvecService(char *machine, char *service);

/* Recoit un message envoye par le serveur.
 * retourne le message ou NULL en cas d'erreur.
 * Note : il faut liberer la memoire apres traitement.
 */
char *Reception();

/* Envoie un message au serveur.
 * Attention, le message doit etre termine par \n
 * renvoie 1 si �a c'est bien pass� 0 sinon
 */
int Emission(char *message);

/* Recoit des donnees envoyees par le serveur.
 * renvoie le nombre d'octets re�us, 0 si la connexion est ferm�e,
 * un nombre n�gatif en cas d'erreur
 */
int ReceptionBinaire(char *donnees, size_t tailleMax);

/* Envoie des donn�es au serveur en pr�cisant leur taille.
 * renvoie le nombre d'octets envoy�s, 0 si la connexion est ferm�e,
 * un nombre n�gatif en cas d'erreur
 */
int EmissionBinaire(char *donnees, size_t taille);

/* Ferme la connexion.
 */
void Terminaison();

/*le sous programme teste les malloc
 *affiche un message de'erreur et exit en cas
 *de probléme
 */

void teste_malloc(char *ptr);

/*le sous programme s'occupe de vider le buffer
*/
void viderBuffer();

/* recupére le login et le mot de passe pour
 *authentifier le client
 *ca renvoie NO_PB si tout s'est bien passé
 et le login de l'utilisateur en parametre
 */

int authentification(char *login);
/*ce sous programme prend en paramettre @ de l'expediteur
 *recuperera les champs necessaire pour l'envoie d'un message
 *et se chargera de l'envoie
 *renverra NO_PB si tout s'est bien passé
*/
void Envoyermessage(char login[]);

/*sous programme de deconnexion
*/
void deconnexion();
/*le sous programme se chargera d'informer
*/
void check();

/*affiche le menu principale
*/
void afficher_menu1();
/*affiche le menu secondaire
*/
void afficher_menu2();
/*sous programme qui se chargera de l'affichage
 * des en-tetes des  messages(nouveaux/tout)
 * elle prendra en parametre new ou all
*/
void list(char *param);
/*ce sous programme prendra en parametre
 *un numero de message a lire puis l'affichera
*/
void lire(char nbre);

#endif
