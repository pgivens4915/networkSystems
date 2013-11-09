// Paul Givens
// November 3, 2013
// Network Systems

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>

#define MAX_PORTS 100

void *serverRoutine(void* port){
    // Declarations
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int clientLength = sizeof(clientAddr);
    int listenfd;
    int connfd;
    int size;
    int* portID = port;
    char buffer [1000];
    printf("Server Port %i\n", *portID);

    // Socket initilization
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(*portID);
    bind(listenfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    // Listening for a connection
    listen(listenfd, 1024);
    connfd = accept(listenfd, (struct sockaddr *)&clientAddr, &clientLength); 

    // Receving message
    size = recvfrom(connfd, buffer, 1000, 0,(struct sockaddr *)&clientAddr,
                    &clientLength);
    printf("%s", buffer);
    pthread_exit(NULL);
}

void *clientRoutine(void* port){
    // Declarations
    int sockfd;
    int* portID = port;
    int connected = 1;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    char buffer[1000] = "Hello\n";
    printf("Checking Port %i\n", *portID);

    // Socket initilization
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(*portID);
    
    // Waiting for a connection to occur 
    //while( connected != 0){
        connected = connect(sockfd, (struct sockaddr *)&serverAddr,
                            sizeof(serverAddr));
    //    sleep(5);
    //}

    // Sending message
    if( connected != 0 ){
        *portID = 0;
        pthread_exit((void*) port);
    }
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&serverAddr,
           sizeof(serverAddr));
    connected = !connected;
    pthread_exit((void*) &connected);
}

int main(int argc, char** argv){
    // Declarations
    pthread_t threads[MAX_PORTS];
    long t;
    int* status;
    char* routerID = argv[1];
    char host;
    char client;
    int hostPort;
    int clientPort;
    int* portArg;
    int* clientArg;
    int weight = 0;
    int threadCount = 0;
    size_t length = 0;
    char * line = NULL;
    FILE *fp;

    // Mallocing
    portArg = (int*) malloc(sizeof(int) * MAX_PORTS);
    clientArg = (int*) malloc(sizeof(int) * MAX_PORTS);

    // Opening the init file
    fp = fopen("init.txt", "r");
    while (getline(&line, &length, fp) != -1){
        // Assuming that the ID has one length only reads valid lines
        if(line[1] == routerID[0]){
            sscanf(line, "<%c,%i,%c,%i,%i>", &host, &hostPort, &client, &clientPort,
                   &weight);
            printf("<%c,%i,%c,%i,%i>\n", host, hostPort, client, clientPort, weight);
            // Checks to see if the port is open
            *(clientArg + threadCount * 4) = clientPort;
            pthread_create(&threads[threadCount], NULL, clientRoutine, (void *) (clientArg + threadCount * 4));
            pthread_join(threads[threadCount], (void*)&status);
            threadCount++;
            // If not create the port
            if (!*status){
                threadCount--;
                *(portArg + threadCount * 4) = hostPort;
                pthread_create(&threads[threadCount], NULL, serverRoutine, (void *) (portArg + threadCount * 4));
                threadCount++;
            }
        }
    }


    printf("before cleanup\n");
    fclose(fp);
    free(line);
    free(portArg);
    pthread_exit(NULL);
    return(0); 
}

