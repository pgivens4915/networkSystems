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
  for(i = 0; i < masterListPoint -1; i++){
    entry = masterList[i];
    strcpy(name, entry.fileData.name);
    strcpy(host, entry.fileData.host);
    size = entry.fileData.size;
    strcpy(ip, inet_ntoa(entry.address));
    port = ntohs(entry.port);

    printf("%s | %i | %s | %s | %u\n", name, size, host, ip, port);
  }
}

void addEntry(struct masterEntry masterList[], int* masterListPoint,
              struct sockaddr_in clientAddr, struct fileEntry fileData){
  masterList[*(masterListPoint)].address = clientAddr.sin_addr;
  masterList[*(masterListPoint)].port = clientAddr.sin_port;
  masterList[*(masterListPoint)].fileData = fileData;
  (*masterListPoint)++;
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
  char mesg[1024] = "TEST\n";

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
    printf("Got packet\n");
    printf("%c\n", mesg[0]);
    switch(mesg[0]){
      case '1':
      entry.clientAddr = clientAddr;
      // Copy the name
      printf("%s Connected\n", (mesg + 1));
      sprintf(entry.name,"%s", (mesg + 1));
      clientList[clientNamePointer] = entry;
      clientNamePointer++;

      // Recieving the table
      size = recvfrom(clientFd, mesg, sizeof(struct fileEntry) * MAX_FILE_COUNT,
                      0, (struct sockaddr *) &clientAddr, &length);
      // Iterating through the table
      printf("Before table point %i\n", size);
      for(i = 0; i <= size; i += sizeof(struct fileEntry)){
        struct fileEntry* currentEntry = (struct fileEntry*) (mesg + i);
        // Register the data in a new table
        addEntry(masterList, &masterListPoint, clientAddr, *currentEntry);
      }
      printMasterTable(masterList, masterListPoint);
      masterListPoint--;

      break;
      // The ls case
      case '2':
      printf("ls requested");
      break;
    }

    sendto(clientFd, mesg, size, 0, (struct sockaddr *) &clientAddr, length);
    
  }
}
