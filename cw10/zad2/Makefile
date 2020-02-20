CC = gcc
FLAGS = -Wall -o
LIBS = -lpthread

SERVER_SOURCE = cw10_datagram_server.c
CLIENT_SOURCE = cw10_datagram_client.c
UTILS_SOURCE = cw10_datagram.c
SERVER_OUT = cw10_datagram_server.x
CLIENT_OUT = cw10_datagram_client.x

all: server client

server: cw10_datagram_server.c cw10_datagram.c
	$(CC) $(FLAGS) $(SERVER_OUT) $(SERVER_SOURCE) $(UTILS_SOURCE) $(LIBS)

client: cw10_datagram_client.c cw10_datagram.c
	$(CC) $(FLAGS) $(CLIENT_OUT) $(CLIENT_SOURCE) $(UTILS_SOURCE) $(LIBS)

clean:
	rm -f *.x
