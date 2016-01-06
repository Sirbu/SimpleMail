CC = gcc
CFLAGS = -std=c99 -Wall

all: Serveur Client

clean:
	rm -Rf *.o Client Serveur 2> /dev/null

### Compilation des fichiers du serveur ###
Serveur: mainServeur.o serveur.o
	$(CC) $(CCFLAGS) mainServeur.o serveur.o -o Serveur

mainServeur.o: mainServeur.c
	$(CC) $(CCFLAGS) -c mainServeur.c -o mainServeur.o

serveur.o:
	$(CC) $(CCFLAGS) -c serveur.c -o serveur.o

### Compilation des fichiers du client ###
Client: mainClient.o client.o
	$(CC) $(CCFLAGS) mainClient.o client.o -o Client

mainClient.o: mainClient.c
	$(CC) $(CCFLAGS) -c mainClient.c -o mainClient.o

client.o:
	$(CC) $(CCFLAGS) -c client.c -o client.o
