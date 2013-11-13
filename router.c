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
#define TOPOLOGY_SIZE 10000
#define true 1
#define false 0

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
int hashes[MAX_EDGES];
char echoBuffer[TOPOLOGY_SIZE];
int echoBuffEnd = 0;

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
            //printf("sizeof %i\n", sizeof(struct tableEntry));
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

// Check to see if the packet is unique
int checkPacket(char* buffer, int size){
    // Declarations
    int hashSum = 0;
    int i = 0;
    pthread_mutex_lock(&mutexsum);
    // My lazy hash
    for(i = 0; i < size; i = i + sizeof(int)){
        hashSum = hashSum + (int) *(buffer + i);
    }
    // Checking to see if the hash exists
    for(i = 0; i < MAX_EDGES && hashes[i] != 0; i++){
        if(hashSum == hashes[i]){
            // If we have already seen it, return true
            return (true);
        }
    }
    // Adding the hash
    hashes[i] = hashSum;
    pthread_mutex_unlock(&mutexsum);
    return(false);
}

// Adds a packet to the buffer stack
void echo(char* buffer, int size){
   pthread_mutex_lock(&mutexsum);
   printf("Before mem size :%i\n", size);
   memcpy((echoBuffer + echoBuffEnd), buffer, size);
   printf("After mem\n");
   echoBuffEnd = echoBuffEnd + size;
   pthread_mutex_unlock(&mutexsum);
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
    int found;
    int bufferPoint = 0;
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
    for(;;){
        size = recvfrom(connfd, buffer, BUFFER_SIZE, 0,
                (struct sockaddr *)&clientAddr, &clientLength);
        // Checks to see if a packet is new
        found = checkPacket(buffer, size);
        if(found){
            printf("Already Seen\n");
        }
        else if (size > 0){
            // Tell the other threads to send
            echo(buffer, size);
            // Move the buffer pointer the appropriate distance
            pthread_mutex_lock(&mutexsum);
            bufferPoint = bufferPoint + size;
            pthread_mutex_unlock(&mutexsum);
        }
        else {
            //printf("size = 0 \n");
            pthread_exit(NULL);
        }
        // If we have not seen this packet yet
        if ((int)*(echoBuffer+bufferPoint) != 0){
            printf("sending echo from port %i\n", *portID);
            pthread_mutex_lock(&mutexsum);
            sendto(connfd,  (echoBuffer + bufferPoint),
                   *(echoBuffer + bufferPoint),0, (struct sockaddr *)&clientAddr,
                   sizeof(clientAddr));
                   
            // Adding the size of the buffer
            bufferPoint = bufferPoint + *(echoBuffer + bufferPoint);
            pthread_mutex_unlock(&mutexsum);
        }
    }

    //printf("Size %i\n", size);
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
    //}

    // Sending message
    if( connected != 0 ){
        *portID = 0;
        pthread_exit((void*) portID);
    }
    size = fillBuffer(buffer, arg->name);
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
    // Zeroing globals
    bzero(hashes, sizeof(int) * MAX_EDGES);
    bzero(echoBuffer, sizeof(char) * TOPOLOGY_SIZE);

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


