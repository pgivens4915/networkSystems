#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#define MAX_NAME_SIZE 64
#define MAX_CLIENTS 10
#define MAX_FILE_COUNT 20

struct fileEntry{
  char name[MAX_NAME_SIZE];
  char host[MAX_NAME_SIZE];
  int port;
  long long size;
};


struct clientEntry{
  char name[MAX_NAME_SIZE];
  struct sockaddr_in clientAddr;
};

struct masterEntry{
  struct fileEntry fileData;
  struct in_addr address;
  uint16_t port;
};

void printMasterTable(struct masterEntry masterList[], int masterListPoint){
  int i;
  char name[MAX_NAME_SIZE];
  char host[MAX_NAME_SIZE];
  char ip[MAX_NAME_SIZE];
  int size;
  int port;
  struct masterEntry entry;
  for(i = 0; i < masterListPoint; i++){
    entry = masterList[i];
    strcpy(name, entry.fileData.name);
    strcpy(host, entry.fileData.host);
    size = entry.fileData.size;
    strcpy(ip, inet_ntoa(entry.address));
    port = entry.fileData.port;

    printf("%s | %i | %s | %s | %i\n", name, size, host, ip, port);
  }
}

void addEntry(struct masterEntry masterList[], int* masterListPoint,
              struct sockaddr_in clientAddr, struct fileEntry fileData){
  printf("In add Entry %i\n", *masterListPoint);
  masterList[*(masterListPoint)].address = clientAddr.sin_addr;
  masterList[*(masterListPoint)].port = clientAddr.sin_port;
  masterList[*(masterListPoint)].fileData = fileData;
  (*masterListPoint)++;
}

void ls(int clientFd, struct sockaddr_in* clientAddr, int* length, 
        struct masterEntry* masterList, int masterListPoint){
  int i;
  masterListPoint++;
  int size = sizeof(struct masterEntry) * masterListPoint;
  sendto(clientFd, &masterListPoint, sizeof(int), 0,
         (struct sockaddr*) clientAddr, *length);
  sendto(clientFd, masterList, size, 0, (struct sockaddr*) clientAddr,
         *length);
}

void removeEntries(char* name, struct masterEntry masterList[],
                   int* masterListPoint){
  int i;
  // Delete everything in the list
  for(i = 0; i < *masterListPoint; i++){
    // If it is the last entry and we need to, just delete it
    if(i == *masterListPoint - 1 && 
        !(strcmp(masterList[i].fileData.host, name))){
      printf("CASE LAST\n");
      printMasterTable(masterList, *masterListPoint);
      (*masterListPoint)--;
    }
    // If we found matching strings
    else if(strcmp(masterList[i].fileData.host, name) == 0){
      printf("CASE found\n");
      printMasterTable(masterList, *masterListPoint);
      masterList[i] = masterList[*masterListPoint - 1];
      // recheck the swap
      i--;
      (*masterListPoint)--;
    }
  }
  printf("Updated List\n");
  printMasterTable(masterList, *masterListPoint);

}

void clientExit(char* name, struct masterEntry masterList[],
                int* masterListPoint){
  printf("Deleting %s\n", name);
  removeEntries(name, masterList, masterListPoint);
}

int main(int argc, char* argv[]){
  int listenFd;
  int clientFd;
  int size;
  int length;
  int clientNamePointer = 0;
  int i;
  int socketId = atoi(argv[1]);
  int masterListPoint = 0;
  struct fileEntry* fileEntryPointer;
  struct clientEntry clientList[MAX_CLIENTS];
  struct clientEntry entry;
  struct sockaddr_in serverAddr;
  struct sockaddr_in clientAddr;
  struct masterEntry masterList[MAX_CLIENTS * MAX_FILE_COUNT];
  socklen_t clientLen;
  char mesg[MAX_FILE_COUNT * sizeof(struct fileEntry)] = "TEST\n";
  char nameDel[MAX_NAME_SIZE];

  listenFd = socket(AF_INET, SOCK_STREAM, 0);

  bzero(&serverAddr, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serverAddr.sin_port = htons(socketId);
  
  bind(listenFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

  listen(listenFd, 1024);

  for(;;){
    clientLen = sizeof(clientAddr);
    clientFd = accept(listenFd, (struct sockaddr *) &clientAddr, &clientLen);
    // Receiving bits
    length = sizeof(clientAddr);
    size = recvfrom(clientFd, mesg, 1024, 0, (struct sockaddr *) &clientAddr,
                    &length);
    switch(mesg[0]){
      case '1':
      entry.clientAddr = clientAddr;
      // Copy the name
      printf("%s Connected\n", (mesg + 1));
      sprintf(entry.name,"%s", (mesg + 1));
      clientList[clientNamePointer] = entry;
      clientNamePointer++;

      // Recieving the table
      size = recvfrom(clientFd, mesg,
                      sizeof(struct fileEntry) * MAX_FILE_COUNT,
                      0, (struct sockaddr *) &clientAddr, &length);
      // Iterating through the table
      printf("before adding %i\n", masterListPoint);
      for(i = 0; i < size; i += sizeof(struct fileEntry)){
        struct fileEntry* currentEntry = (struct fileEntry*) (mesg + i);
        printf("size %i\n", currentEntry->size);
        // Register the data in a new table
        addEntry(masterList, &masterListPoint, clientAddr, *currentEntry);
      }
      printMasterTable(masterList, masterListPoint);

      break;
      // The ls case
      case '2':
      ls(clientFd, &clientAddr, &length, masterList, masterListPoint);
      break;
      case '3':
      strcpy(nameDel, (mesg+1));
      clientExit(nameDel, masterList, &masterListPoint);
      break;
    }

    sendto(clientFd, mesg, size, 0, (struct sockaddr *) &clientAddr, length);
    
  }
}
