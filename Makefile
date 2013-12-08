all: Server Client
	cp Client ./test1
	cp Client ./test2

Server: server.c
	gcc server.c -o Server

Client: client.c
	gcc client.c -o Client
