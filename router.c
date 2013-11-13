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
#define MAX_EDGES 100
#define BUFFER_SIZE 1000

// Structure creation
struct Argument {
    char name;
    int port;
};

struct tableEntry {
    char host;
    char target;
    int weight;
    int fromPort;
    int toPort;
};


// Nasty global declarations
pthread_mutex_t mutexsum;
int tableEntry = 0;
struct tableEntry table[MAX_EDGES]; 

void printBuffer(char* buffer){
    // Declarations
    int count = 0;
    int i = 0;
    struct tableEntry* thisEntry;

    // The first part of the buffer has the count
    count = (int) *buffer;
    for(i = 0; i < count; i++){
        // Pulls the table entry out of memory
        thisEntry = (struct tableEntry*) (buffer + 4 + (i * 16));

        printf("<%c,%i,%c,%i,%i>\n", thisEntry->host, thisEntry->fromPort,
               thisEntry->target, thisEntry->toPort, thisEntry->weight);
    }

}

// Fills the buffer with a packet that states the number of entries followed by
// some entries of the form struct tableEntry
int fillBuffer(char* buffer, char host){
    int count = 0;
    int i = 0;

    // For each local link in the table
    pthread_mutex_lock(&mutexsum);
    for(i = 0; i < tableEntry; i++){
        if(table[i].host == host){
            printf("sizeof %i\n", sizeof(tableEntry));
            //printf("tableInfo: %i\n", table[i].fromPort);
            count++;
            // Copying a table entry into the buffer the 4 is an int in the front
            // For some reason sizeof returns 2 instead of 16, that is why 16 
            // Is here, it is the sizeof the struct
            memcpy((void*) (buffer + 4 + (i * 16)), 
                   (void*) &(table[i]), 16);

        }
    }
    memcpy((void*) buffer, (void*) &count, sizeof(int));
    printBuffer(buffer);
    pthread_mutex_unlock(&mutexsum);
    return (4 + count * 16);

}
void *serverRoutine(void* port){
    // Declarations
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int clientLength = sizeof(clientAddr);
    int listenfd;
    int connfd;
    int size;
    int* portID = port;
    char buffer [BUFFER_SIZE];
    // printf("Server Port %i\n", *portID);

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
    size = recvfrom(connfd, buffer, BUFFER_SIZE, 0,
                    (struct sockaddr *)&clientAddr, &clientLength);

    printf("Size %i\n", size);
    printBuffer(buffer);
    // printf("%s", buffer);
    pthread_exit(NULL);
}

void *clientRoutine(void* port){
    // Declarations
    int sockfd;
    int* portID = port;
    int connected = 1;
    int size = 0;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    char buffer[BUFFER_SIZE];
    //printf("Checking Port %i\n", *portID);

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
    size = fillBuffer(buffer, 'F');
    sendto(sockfd, buffer, size, 0, (struct sockaddr *)&serverAddr,
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
    
    // Initing mutex
    pthread_mutex_init(&mutexsum, NULL);

    // Mallocing
    portArg = (int*) malloc(sizeof(int) * MAX_PORTS);
    clientArg = (int*) malloc(sizeof(int) * MAX_PORTS);

    // Opening the init file
    fp = fopen("init.txt", "r");
    while (getline(&line, &length, fp) != -1){
        // Assuming that the ID has one length only reads valid lines
        if(line[1] == routerID[0]){
            sscanf(line, "<%c,%i,%c,%i,%i>", &host, &hostPort, &client, 
                   &clientPort, &weight);

            //printf("<%c,%i,%c,%i,%i>\n", host, hostPort, client, 
            //       clientPort, weight);

            // Assigning a table entry
            pthread_mutex_lock(&mutexsum);
            table[threadCount].host = host;
            table[threadCount].target = client;
            table[threadCount].weight = weight;
            table[threadCount].fromPort = hostPort;
            table[threadCount].toPort = clientPort;
            tableEntry++;
            pthread_mutex_unlock(&mutexsum);

            // Checks to see if the port is open
            *(clientArg + threadCount * 4) = clientPort;
            pthread_create(&threads[threadCount], NULL, clientRoutine,
                           (void *) (clientArg + threadCount * 4));

            pthread_join(threads[threadCount], (void*)&status);
            threadCount++;
            // If not create the port
            if (!*status){
                threadCount--;
                *(portArg + threadCount * 4) = hostPort;
                pthread_create(&threads[threadCount], NULL, serverRoutine,
                               (void *) (portArg + threadCount * 4));

                threadCount++;
            }
        }
    }


    // Cleanup
    pthread_mutex_destroy(&mutexsum);
    fclose(fp);
    free(line);
    free(portArg);
    pthread_exit(NULL);
    return(0); 
}


