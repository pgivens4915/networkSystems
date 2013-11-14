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

struct Return {
    int sockfd;
    struct sockaddr_in serverAddr;
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

void fillBuffer(char* buffer, char host){
    int count = 0;
    int i = 0;

    // For each local link in the table
    pthread_mutex_lock(&mutexsum);
    for(i = 0; i < tableEntry; i++){
        if(table[i].host == host){
            // Copying a table entry into the buffer
            memcpy((void*) (buffer + i * sizeof(struct tableEntry)), 
                   (void*) &table[i], sizeof(tableEntry));
        }
    }
    pthread_mutex_unlock(&mutexsum);

}
void *serverRoutine(void* port){
    // Declarations
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    struct Return returnArg;
    int clientLength = sizeof(clientAddr);
    int listenfd;
    int connfd;
    int size;
    int portID = *((int*) port);
    char buffer [BUFFER_SIZE];
    //printf("Server Port %i\n", portID);

    // Socket initilization
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(portID);
    bind(listenfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    // Listening for a connection
    listen(listenfd, 1024);
    connfd = accept(listenfd, (struct sockaddr *)&clientAddr, &clientLength); 

    // Receving message
    size = recvfrom(connfd, buffer, BUFFER_SIZE, 0,(struct sockaddr *)&clientAddr,
                    &clientLength);
    //printf("Server %i connected\n", portID);
    returnArg.sockfd = connfd;
    returnArg.serverAddr = clientAddr;
    pthread_exit((void*) &returnArg);
}

void *clientRoutine(void* port){
    // Declarations
    struct Return returnArg;
    int sockfd;
    int* portID = port;
    int connected = 1;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    char buffer[BUFFER_SIZE] = "Connected";
    //printf("Checking Port %i\n", *portID);

    // Socket initilization
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(*portID);
    
    connected = connect(sockfd, (struct sockaddr *)&serverAddr,
            sizeof(serverAddr));

    // Sending message
    if( connected != 0 ){
        pthread_exit(NULL);
    }
    //fillBuffer(&buffer, 'A');
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&serverAddr,
           sizeof(serverAddr));
    //printf("Client %i connected\n", *portID);
    connected = !connected;
    returnArg.sockfd = sockfd;
    returnArg.serverAddr = serverAddr;
    pthread_exit((void*) &returnArg);
}

int main(int argc, char** argv){
    // Declarations
    pthread_t threads[MAX_PORTS];
    struct Return returnArg[MAX_PORTS];
    long t;
    struct Return* status;
    char* routerID = argv[1];
    char host;
    char client;
    int hosts[MAX_PORTS];
    int i;
    int pid = 0;
    int hostCount = 0;
    int hostPort;
    int clientPort;
    int returnCount = 0;
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

            printf("<%c,%i,%c,%i,%i>\n", host, hostPort, client, 
                   clientPort, weight);

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
            if (!status){
                threadCount--;
                *(portArg + threadCount * 4) = hostPort;
                pid = pthread_create(&threads[threadCount], NULL, serverRoutine,
                               (void *) (portArg + threadCount * 4));
                hosts[hostCount] = threads[threadCount];
                hostCount++;

                threadCount++;
            }
            else{
                //printf("Client status %i\n", *status);
                returnArg[returnCount] = *status;
                returnCount++;
            }
        }
    }
    
    // Getting the arguments
    for(i = 0; i < hostCount; i++){
        pthread_join(hosts[i], (void*)&status);
        //printf("Host status %i\n", *status);
        returnArg[returnCount] = *status;
        returnCount++;
    }
    printf("Done with Initilization\n");
    
    for( i = 0; i < returnCount; i++){
        printf("returnArg : %i\n", returnArg[i].sockfd);
    }

    //for(;;){
    //    // Send data from each port
    //    sleep(5);
    //    for( i = 0; i < returnCount; i++){
    //        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&serverAddr,
    //                sizeof(serverAddr));
    //    }

        // Select with a 5 sec timeout
    //}


    // Cleanup
    pthread_mutex_destroy(&mutexsum);
    fclose(fp);
    free(line);
    free(portArg);
    pthread_exit(NULL);
    return(0); 
}


