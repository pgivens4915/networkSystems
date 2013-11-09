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
    printf("Host Port %i\n", *portID);

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
    printf("Server Port %i\n", *portID);

    // Socket initilization
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(*portID);
    
    // Waiting for a connection to occur 
    while( connected != 0){
        connected = connect(sockfd, (struct sockaddr *)&serverAddr,
                            sizeof(serverAddr));
        sleep(5);
    }
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&serverAddr,
           sizeof(serverAddr));

    // Sending message
    pthread_exit(NULL);
}

int main(int argc, char** argv){
    // Declarations
    pthread_t threads[10];
    long t;
    void* status;
    char* routerID = argv[1];
    char host;
    char client;
    int hostPort = 0;
    int clientPort = 0;
    int weight = 0;
    size_t length = 0;
    char * line = NULL;
    FILE *fp;

    // Opening the init file
    fp = fopen("init.txt", "r");
    while (getline(&line, &length, fp) != -1){
        // Assuming that the ID has one length only reads valid lines
        if(line[1] == routerID[0]){
            sscanf("<%c,%i,%c,%i,%i>", &host, &hostPort, &client, &clientPort,
                   &weight)
            print
            
        }
    }

    pthread_create(&threads[0], NULL, serverRoutine, (void *) &local);
    pthread_create(&threads[1], NULL, clientRoutine, (void *) &client);

    // Waiting for a join
    pthread_join(threads[0], &status);
    pthread_join(threads[1], &status);

    fclose(fp);
    free(line);
    pthread_exit(NULL);
    return(0);

}

