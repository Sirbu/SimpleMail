CC = gcc
CFLAGS = -std=c99 -Wall

all: Serveur Client

clean:
	rm -Rf *.o Client Serveur 2> /dev/null

### Compilation des fichiers du serveur ###
Serveur: mainServeur.o serveur.o
	$(CC) $(CFLAGS) mainServeur.o serveur.o -o Serveur

mainServeur.o: mainServeur.c
	$(CC) $(CFLAGS) -c mainServeur.c -o mainServeur.o

serveur.o:
	$(CC) $(CFLAGS) -c serveur.c -o serveur.o

### Compilation des fichiers du client ###
Client: mainClient.o client.o
	$(CC) $(CFLAGS) mainClient.o client.o -o Client

mainClient.o: mainClient.c
	$(CC) $(CFLAGS) -c mainClient.c -o mainClient.o

client.o:
	$(CC) $(CFLAGS) -c client.c -o client.o
