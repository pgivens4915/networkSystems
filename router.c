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
        thisEntry = (struct tableEntry*) (buffer + sizeof(int)+ 
                    (i * sizeof(struct tableEntry)));

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
            printf("sizeof %i\n", sizeof(struct tableEntry));
            //printf("tableInfo: %i\n", table[i].fromPort);
            count++;
            // Copying a table entry into the buffer the 4 is an int in the front
            memcpy((void*) (buffer + sizeof(int) + (i * sizeof(struct tableEntry))), 
                   (void*) &(table[i]), sizeof(struct tableEntry));

        }
    }
    memcpy((void*) buffer, (void*) &count, sizeof(int));
    printBuffer(buffer);
    pthread_mutex_unlock(&mutexsum);
    return (sizeof(int) + count * sizeof(struct tableEntry));

}
void *serverRoutine(void* argue){
    // Declarations
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    struct Argument* arg = argue;
    int clientLength = sizeof(clientAddr);
    int listenfd;
    int connfd;
    int size;
    int* portID = &(arg->port);
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

void *clientRoutine(void* argue){
    // Declarations
    struct Argument* arg = argue;
    int sockfd;
    int* portID = &(arg->port);
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
        pthread_exit((void*) portID);
    }
    size = fillBuffer(buffer, arg->name );
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
    struct Argument* portArg;
    struct Argument* clientArg;
    struct Argument newStruct;
    int weight = 0;
    int threadCount = 0;
    size_t length = 0;
    char * line = NULL;
    FILE *fp;
    
    // Initing mutex
    pthread_mutex_init(&mutexsum, NULL);

    // Mallocing
    portArg = (struct Argument*) malloc(sizeof(struct Argument) * MAX_PORTS);
    clientArg = (struct Argument*) malloc(sizeof(struct Argument) * MAX_PORTS);

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
            // Create the struct
            newStruct.port = clientPort;
            newStruct.name = line[1];
            *(clientArg + threadCount * sizeof(struct Argument)) = newStruct;

            pthread_create(&threads[threadCount], NULL, clientRoutine,
                    (void *) (clientArg + threadCount * 
                            sizeof(struct Argument)));

            pthread_join(threads[threadCount], (void*)&status);
            threadCount++;
            // If not create the port
            if (!*status){
                threadCount--;
                // Creating a structure with the host port and router ID
                newStruct.port = hostPort;
                newStruct.name = line[1];
                *(portArg + threadCount * sizeof(struct Argument)) = newStruct;
                
                pthread_create(&threads[threadCount], NULL, serverRoutine,
                        (void *) (portArg + threadCount * 
                                sizeof(struct Argument)));

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


