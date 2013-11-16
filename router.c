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
#include <errno.h>

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
FILE *OUTPUT;

// Prints a buffer
void printBuffer(char* buffer){
    // Declarations
    int count = 0;
    int i = 0;
    struct tableEntry* thisEntry;

    // The first part of the buffer has the count
    fprintf(OUTPUT, "PACKET RECEIVED\n");
    count = (int) *buffer;
    for(i = 0; i < count; i++){
        // Pulls the table entry out of memory
        thisEntry = (struct tableEntry*) (buffer + sizeof(int)+
                (i * sizeof(struct tableEntry)));

        fprintf(OUTPUT, "<%c,%i,%c,%i,%i>\n", thisEntry->host, thisEntry->fromPort,
                thisEntry->target, thisEntry->toPort, thisEntry->weight);
    }
    fprintf(OUTPUT, "PACKET END\n\n");
}

void fillBuffer(char* buffer, char host){
    int count = 0;
    int i = 0;

    // For each local link in the table
    pthread_mutex_lock(&mutexsum);
    for(i = 0; i < tableEntry; i++){
        if(table[i].host == host){
            count++;
            // Copying a table entry into the buffer
            memcpy((void*) (buffer + sizeof(int)+ i * sizeof(struct tableEntry)), 
                    (void*) &table[i], sizeof(struct tableEntry));
        }
    }
    memcpy((void*) buffer, (void*)&count, sizeof(int));

    pthread_mutex_unlock(&mutexsum);

}

void printGraph(struct tableEntry* table, int tableSize){
    int i = 0;
    struct tableEntry* thisEntry;
    fprintf(OUTPUT, "GRAPH STATE\n");
    for(i = 0; i < tableSize; i++){
        thisEntry = (table + i * sizeof(struct tableEntry));
        fprintf(OUTPUT, "<%c,%i,%c,%i,%i>\n", thisEntry->host, thisEntry->fromPort,
                thisEntry->target, thisEntry->toPort, thisEntry->weight);

    }
    fprintf(OUTPUT, "END STATE\n\n");
    
}

// Fills a graph with a buffer, and sees if the data already exists
// returns zero if already seen, otherwise the size of the new table
int fillGraph(struct tableEntry* table, int* tableSize, char* buffer){
    int i = 0;
    int j = 0;
    int result = 0;
    void* first;
    void* second;
    struct tableEntry thisEntry;
    // Checking to see if there are any repeats
    // For each table entry
    for(i = 0; i < *tableSize; i++){
        // For each buffer entry
        for(j = 0; j < *((int*)buffer); j++){
            first = (void*)(table + i * sizeof(struct tableEntry));
            second = (void*)(buffer + sizeof(int) + j*sizeof(struct tableEntry));
            result = memcmp(first,second, sizeof(struct tableEntry));
            if(result == 0){
                return 0;
            }
        }
    }
    if(buffer == NULL) printf("arrrradsfasd\n");

    // Since we have not seen the data copy it to the end of the table
    for(i = *tableSize; i < *tableSize + *((int*)buffer); i++){
        thisEntry = *((struct tableEntry*) (buffer + sizeof(int) + (i - *tableSize) * sizeof(struct tableEntry)));

        *(table + i * sizeof(struct tableEntry)) = thisEntry;
    }

    // The new table size = the old table size, plus number of new entries
    *tableSize = *tableSize + *((int*)buffer);



    return *tableSize;


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

    returnArg.sockfd = connfd;
    returnArg.serverAddr = clientAddr;
    struct sockaddr_in * DEBUG = &clientAddr;
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
    int serverLeng = sizeof(serverAddr);

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
    returnArg.sockfd = sockfd;
    returnArg.serverAddr = serverAddr;
    struct sockaddr_in * DEBUG = &serverAddr;
    pthread_exit((void*) &returnArg);
}

// Finds the distance of between two nodes
int pathDist(int source, int target, struct tableEntry* graph, int graphSize){
    struct tableEntry thisEntry;
    char targetChar = source + 'A';
    char sourceChar = target + 'A';
    int i;
    if(source == target){
        return (0);
    }
    for(i = 0; i < graphSize; i++){
        thisEntry = *(graph + i * sizeof(struct tableEntry));
        if(thisEntry.host == sourceChar && thisEntry.target == targetChar){
            return thisEntry.weight;
        }
    }
    // If nothing is found
    return(0);


}

// Finds the smallest array entry > intTarget
int smallest(int* distance, int intTarget){
    int i;
    int smallest = 100000;
    int current;
    int index = -1;
    int targetWeight = *(distance + intTarget * sizeof(int));
    for(i = 0; i < 6; i++){
        current = *(distance + i * sizeof(int));
        if(current < smallest && (current > targetWeight || (current == targetWeight && i > intTarget))){
            smallest = current;
            index = i;
        }

    }
    return index;
}

char findNode(char* direction, char start, char current){
   int index = current - 'A';
   char pointsTo = *(direction + index * sizeof(char)); 
   int pointInt = pointsTo - 'A';
   printf("START %c\n", start);
   printf("%c-->%c\n", current, pointsTo);
   // Base case, we are one hop away from the target
   printf("this %c\n", *(direction + pointInt * sizeof(char)));
   if (pointsTo == start){
       printf("BASE CASE REACHED\n");
       return current;
   }
   else{
       if (current == 'A' || current == 'B' ||current == 'C' ||current == 'D' 
               ||current == 'E' ||current == 'F'){
           return (findNode(direction, start, *(direction + index * sizeof(char))));
       }
       else return(0);
   }
}

// Does dijkstras alg on a list
void dijkstra(struct tableEntry* graph, int graphSize, int* distance, char start,
        char* direction){

    char target = start;
    char port;
    int i;
    int intTarget = start - 'A';
    int path;
    int node;
    int nodeDist;
    for(i = 0; i < 6; i++){
        *(distance + i * sizeof(int)) = 10000;
    }
    // Setting start to zero
    *(distance + intTarget * sizeof(int)) = 0;
    *(direction + intTarget * sizeof(char)) = start;

    // Now we loop until done
    for(i = 1; i < 6; i++){
        for(node = 0; node < 6; node++){
            path = pathDist(node, intTarget, graph, graphSize);
            if (path != 0){
                nodeDist = *(distance + intTarget * sizeof(int));
                if (path + nodeDist < (*(distance + node * sizeof(int)) )){
                    *(distance + node * sizeof(int)) =  path + nodeDist;
                    // Adding the direction
                    if( 'A' + intTarget == start){
                        printf("in here with %c\n", node + 'A');
                        *(direction + node * sizeof(char)) = start;
                    }
                    else{
                        port = findNode(direction, start, 'A' + intTarget);
                        *(direction + node * sizeof(char)) = port;
                    }

                }
            }
        }
        // find the smallest node bigger than the last target
        intTarget = smallest(distance, intTarget);
        if (intTarget == -1){
            printf("Dijkstras fail\n");
            return;

        }
    }
    for(i = 1; i < 6; i++){
        if(*(direction + i * sizeof(char)) == start){
           *(direction + i * sizeof(char)) = 'A' + i; 
        }
    }

}

void printDistance(int* distance, char* direction){
    fprintf(OUTPUT, "DIJEKSTRAS\n");
    fprintf(OUTPUT, "A:%i B:%i C:%i D:%i E:%i F:%i\n", *distance, *(distance + 1 * sizeof(int)),
            *(distance + 2 * sizeof(int)),*(distance + 3 * sizeof(int)),
            *(distance + 4 * sizeof(int)),*(distance + 5 * sizeof(int)));
    printf("A:%i B:%i C:%i D:%i E:%i F:%i\n", *distance, *(distance + 1 * sizeof(int)),
            *(distance + 2 * sizeof(int)),*(distance + 3 * sizeof(int)),
            *(distance + 4 * sizeof(int)),*(distance + 5 * sizeof(int)));
    fprintf(OUTPUT, "A:%c B:%c C:%c D:%c E:%c F:%c\n", *direction, *(direction+ 1 * sizeof(char)),
            *(direction + 2 * sizeof(char)),*(direction + 3 * sizeof(char)),
            *(direction + 4 * sizeof(char)),*(direction+ 5 * sizeof(char)));
    fprintf(OUTPUT, "END DIJEKSTRAS\n\n");
    printf("A:%c B:%c C:%c D:%c E:%c F:%c\n", *direction, *(direction+ 1 * sizeof(char)),
            *(direction + 2 * sizeof(char)),*(direction + 3 * sizeof(char)),
            *(direction + 4 * sizeof(char)),*(direction+ 5 * sizeof(char)));
}

int main(int argc, char** argv){
    // Declarations
    pthread_t threads[MAX_PORTS];
    struct Return returnArg[MAX_PORTS];
    struct tableEntry graph[MAX_EDGES * 10];
    long t;
    struct Return* status;
    char* routerID = argv[1];
    char* buffer[BUFFER_SIZE];
    char host;
    char client;
    int hosts[MAX_PORTS];
    int distance[6];
    int tableSize = 0;
    int i;
    int pid = 0;
    int hostCount = 0;
    int hostPort;
    int clientPort;
    int returnCount = 0;
    int* portArg;
    int* clientArg;
    int weight = 0;
    int timeout = 0;
    int threadCount = 0;
    int check;
    int clientLength;
    size_t length = 0;
    char * line = NULL;
    char * direction;
    FILE *fp;

    // Initing mutex
    pthread_mutex_init(&mutexsum, NULL);

    // Mallocing
    portArg = (int*) malloc(sizeof(int) * MAX_PORTS);
    clientArg = (int*) malloc(sizeof(int) * MAX_PORTS);
    direction = (char*) malloc(sizeof(char) * MAX_PORTS);

    // Opening the init file
    fp = fopen(argv[2], "r");
    OUTPUT = fopen(argv[3], "w");
    if (OUTPUT == NULL){
        printf("can not open file\n");
        return(0);
    }
    fprintf(OUTPUT, "data\n");
    while (getline(&line, &length, fp) != -1){
        // Assuming that the ID has one length only reads valid lines
        if(line[1] == routerID[0]){
            sscanf(line, "<%c,%i,%c,%i,%i>", &host, &hostPort, &client, 
                    &clientPort, &weight);

            printf("<%c,%i,%c,%i,%i>\n", host, hostPort, client, 
                    clientPort, weight);

            // Assigning a table entry
            pthread_mutex_lock(&mutexsum);
            table[tableEntry].host = host;
            table[tableEntry].target = client;
            table[tableEntry].weight = weight;
            table[tableEntry].fromPort = hostPort;
            table[tableEntry].toPort = clientPort;
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
                returnArg[returnCount] = *status;
                returnCount++;
            }
        }
    }

    // Getting the arguments
    for(i = 0; i < hostCount; i++){
        pthread_join(hosts[i], (void*)&status);
        returnArg[returnCount] = *status;
        returnCount++;
    }
    printf("Done with Initilization\n");

    clientLength = sizeof(struct sockaddr_in);
    //for(;;){
    // Send data from each port
    //sleep(5);
    fillBuffer((char*)buffer, routerID[0]);
    check = fillGraph(graph, &tableSize, (char*)buffer);
    printGraph(graph, tableSize);
    dijkstra(graph,tableSize, (int*)distance, routerID[0], direction); 
    printDistance(distance, direction);
    for( i = 0; i < returnCount; i++){
        check = sendto(returnArg[i].sockfd, buffer,
                *((int*) buffer) * sizeof(struct tableEntry) + sizeof(int), 0,
                (struct sockaddr *)&(returnArg[i].serverAddr),
                sizeof(struct sockaddr_in));

        if (check == -1) {
            int mistake = errno;
            printf("%s", strerror(mistake));
            printf("ARRRRRG\n");
        }
    }
    for( i = 0; i < returnCount; i++){
        struct sockaddr_in thisAddr = returnArg[i].serverAddr;
        char* bleh = malloc(100 * sizeof(char));
        check = recvfrom(returnArg[i].sockfd, bleh, BUFFER_SIZE, 0,
                (struct sockaddr *)&thisAddr,
                &clientLength);
        printf("packet start\n\n");
        printBuffer(bleh);
        printf("\npacket end\n");
        if(check > 0){
            printf("filling Graph\n");
            check = fillGraph(graph, &tableSize, bleh);
            if(check != 0){
                printGraph(graph, tableSize);
                dijkstra(graph,tableSize, (int*)distance, routerID[0], direction); 
                printDistance(distance, direction);
                int j;
                // send on all ports minus current port
                for(j = 0; j < returnCount; j++){
                    if(returnArg[i].sockfd != returnArg[j].sockfd){
                        printf("echo\n");
                        check = sendto(returnArg[j].sockfd, bleh,
                                *((int*) bleh) * sizeof(struct tableEntry) +
                                sizeof(int), 0,
                                (struct sockaddr *)&(returnArg[i].serverAddr),
                                sizeof(struct sockaddr_in));
                        if(check < 1){
                            printf("FAIL %s\n", strerror(errno));
                        }
                    }
                }
            }
            else printf("seen\n");
        }
        else{
            printf("FAIL %s \n", strerror(errno));
        }

    }
    int biggest = 0;
    int retval;
    fd_set readfds;
    struct timeval tv;

    FD_ZERO(&readfds);
    // Setting fd set
    FD_SET(0,&readfds);
    printf("RETURNCOUNT %i\n", returnCount);
    for( i = 0; i < returnCount; i++){
        FD_SET(returnArg[i].sockfd, &readfds);
        printf("sockfd %i\n",returnArg[i].sockfd);
        if (returnArg[i].sockfd > biggest){
            biggest = returnArg[i].sockfd;
            printf("biggest %i\n", returnArg[i].sockfd);
        }
    }

    
    for(;;){

        // Setting timeout
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        retval = select(biggest + 1, &readfds, NULL, NULL, &tv);
        printf("one loop\n");
        for( i = 0; i < returnCount; i++){
           if(FD_ISSET(returnArg[i].sockfd, &readfds)){
                printf("I heard socket %i\n", i);
                char* bleh = malloc(100 * sizeof(char));
                struct sockaddr_in thisAddr = returnArg[i].serverAddr;
                check = recvfrom(returnArg[i].sockfd, bleh, BUFFER_SIZE, 0,
                        (struct sockaddr *)&thisAddr,
                        &clientLength);
                if(check > 0){
                    check = fillGraph(graph, &tableSize, bleh);
                    printf("packet start select\n\n");
                    printBuffer(bleh);
                    printf("\npacket end select\n");
                    if(check != 0){
                        printGraph(graph, tableSize);
                        dijkstra(graph,tableSize, (int*)distance, routerID[0], direction); 
                        printDistance(distance, direction);
                        int j;
                         //send on all ports minus current port
                        for(j = 0; j < returnCount; j++){
                            if(returnArg[i].sockfd != returnArg[j].sockfd){
                                printf("echo\n");
                                check = sendto(returnArg[j].sockfd, bleh,
                                        *((int*) bleh) * sizeof(struct tableEntry) +
                                        sizeof(int), 0,
                                        (struct sockaddr *)&(returnArg[i].serverAddr),
                                        sizeof(struct sockaddr_in));

                            }
                        }
                    }
                    else printf("SEEN DAT\n");

                } else {
                    printf("FAIL %s\n", strerror(errno));
                    return(0);
                }
            }

        }
        printf("END LOOP\n");
        if(retval == 0){
            // Sending the link state
            if(timeout > 1){
                printf("exit\n");
                fclose(OUTPUT);
                printf("closed\n");
                fclose(fp);
                return(0);
            }
            timeout++;
 
            printf("TIMEOUT\n");
            fillBuffer((char*)buffer, routerID[0]);
            for( i = 0; i < returnCount; i++){
                check = sendto(returnArg[i].sockfd, buffer,
                        *((int*) buffer) * sizeof(struct tableEntry) + sizeof(int), 0,
                        (struct sockaddr *)&(returnArg[i].serverAddr),
                        sizeof(struct sockaddr_in));

                if (check == -1) {
                    int mistake = errno;
                    printf("%s", strerror(mistake));
                    printf("ARRRRRG\n");
                }
            }
 
            sleep(5);
        }
        
    }

        // Now the initial message has been flooded and recived

        // Cleanup
        pthread_mutex_destroy(&mutexsum);
        fclose(OUTPUT);
        fclose(fp);
        free(line);
        free(portArg);
        pthread_exit(NULL);
        return(0); 
    }


