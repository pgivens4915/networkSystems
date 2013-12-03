all: Server Client

Server: server.c
	gcc server.c -o Server

Client: client.c
	gcc client.c -o Client
