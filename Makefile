CC = gcc
CFLAGS = -Wall -Werror -g 

all: main client attacker

main: server.o
	$(CC) $(CFLAGS) -o main server.o

client: client.o
	$(CC) $(CFLAGS) -o client client.o

attacker: attacker.o
	$(CC) $(CFLAGS) -o attacker attacker.o -lm

server.o: server.c
	$(CC) $(CFLAGS) -c server.c

client.o: client.c
	$(CC) $(CFLAGS) -c client.c

attacker.o: attacker.c
	$(CC) $(CFLAGS) -c attacker.c

clean:
	rm -f main client attacker server.o client.o attacker.o