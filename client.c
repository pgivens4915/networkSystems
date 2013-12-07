#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#define MAX_NAME_SIZE 64
#define MAX_FILE_COUNT 20
#define MAX_CLIENTS 10

struct fileEntry{
  char name[MAX_NAME_SIZE];
  char host[MAX_NAME_SIZE];
  int port;
  long long size;
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
    port = entry.fileData.port;;

    printf("%s | %i | %s | %s | %i\n", name, size, host, ip, port);
  }
}



void ls(int serverFd, int size, struct sockaddr_in* serverAddr, 
        struct masterEntry* masterList, int* masterListPoint){
  // Packet identifier for ls == '2'
  char packet = '2';
  int length;
  int maxLength;
  sendto(serverFd, &packet, sizeof(char), 0, (struct sockaddr*) serverAddr,
         size);
  // Geting the pointer
  recvfrom(serverFd, masterListPoint, sizeof(int), 0,
           (struct sockaddr*) serverAddr, &size);
  // Getting the table
  maxLength = sizeof(struct masterEntry) * MAX_FILE_COUNT * MAX_CLIENTS;
  length = recvfrom(serverFd, masterList, maxLength, 0,
           (struct sockaddr*) serverAddr, &size);
  printf("length %i\n", length);
  printMasterTable(masterList, *masterListPoint);
}

void registerName(int serverFd, struct sockaddr_in* serverAddr, int size,
                  char* name, int transferPort){
  DIR *dir;
  struct stat st;
  struct dirent *ent;
  long long fileSize;
  struct fileEntry fileTable[MAX_FILE_COUNT];
  int fileTablePointer = 0;
  // 1 is the packet number that lets the server know
  // we are registering a name
  char packet[MAX_NAME_SIZE + 1] = "1";
  // Appending the strings
  strcat(packet, name);
  // Sending the string!
  sendto(serverFd, packet, MAX_NAME_SIZE + 1, 0,
        (struct sockaddr*) serverAddr, size);
  // Local file info
  if ((dir = opendir(".")) == NULL){
    perror("DIRECTORY ERROR");
    exit(1);
  }
  // Getting the names and sizes
  while((ent = readdir(dir)) != NULL){
    // If it is not a hidden file
    if((ent->d_name)[0] != '.'){
      printf("%s ", ent->d_name);
      stat(ent->d_name, &st);
      fileSize = (long long) st.st_size;
      printf("%i\n", (int)st.st_size);
      // Copying the file name into the table
      strcpy(fileTable[fileTablePointer].name, ent->d_name);
      // copying the hostname
      strcpy(fileTable[fileTablePointer].host, name);
      fileTable[fileTablePointer].size = fileSize;
      fileTable[fileTablePointer].port = transferPort ;
      fileTablePointer++;
    }
  }
  // Sending the table
  sendto(serverFd, fileTable, sizeof(struct fileEntry) * fileTablePointer, 0,
        (struct sockaddr*) serverAddr, size);

}

// Resolves the address of a needed file
struct sockaddr_in resolveAddress(struct masterEntry masterList[], 
                                  int masterListPoint, char* name){
  int i;
  struct sockaddr_in addr;
  // Looking through the list
  for(i = 0; i < masterListPoint; i++){
    if(strcmp(masterList[i].fileData.name, name) == 0){
      addr.sin_family = AF_INET;  
      addr.sin_addr = masterList[i].address; 
      addr.sin_port = htons(masterList[i].fileData.port);
      return(addr);
    }
  }
  printf("File not found\n");
  addr.sin_port = 0;
  return(addr);
}

// Runs the get command
void get(struct masterEntry masterList[], int masterListPoint){
  char fileName[MAX_NAME_SIZE];
  char mesg[10] = "echo\n";
  int fileSock;
  struct sockaddr_in fileAddr;
  printf("Enter filename :\n");
  scanf("%s", fileName);
  fileAddr = resolveAddress(masterList, masterListPoint, fileName);
  // If file not found
  if(fileAddr.sin_port == 0){
    return;
  }
  // Attempt to connect to the client with the file
  fileSock = socket(AF_INET, SOCK_STREAM, 0);
  connect(fileSock, (struct sockaddr*) &fileAddr, sizeof(fileAddr));
  sendto(fileSock, mesg, strlen(mesg), 0, (struct sockaddr*) &fileAddr,
         sizeof(fileAddr));
  printf("sent\n");
}

int main(int argc, char* argv[]){
  char* message;
  message = malloc(1024);
  char name[MAX_NAME_SIZE];
  int plusOne = 1;
  int listenFd;
  int serverFd;
  int size;
  int portNumber;
  int masterListPoint = 0;
  int transferPort;
  int transferFd;
  struct sockaddr_in serverAddr;
  struct sockaddr_in transferAddr;
  struct masterEntry masterList[MAX_CLIENTS * MAX_FILE_COUNT];
  fd_set master;
  fd_set read_fds;
  size_t length;

  // Copying name from command line
  strcpy(name, argv[1]);
  // Getting the port number
  portNumber = atoi(argv[2]);
  transferPort = atoi(argv[3]);

  // DEBUG message
  sprintf(message, "SENDING\n");
  // End debug messge
  
  
  // Connecting to the server
  bzero(&serverAddr, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serverAddr.sin_port = htons(portNumber);

  serverFd = socket(AF_INET, SOCK_STREAM, 0);
  connect(serverFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

  size = sizeof(serverAddr);
  // Registering the clients name
  registerName(serverFd, &serverAddr, size, name, transferPort);
  close(serverFd);

  // Opening a port for file transfer
  transferFd = socket(AF_INET, SOCK_STREAM, 0);
  bzero(&transferAddr, sizeof(transferAddr));
  transferAddr.sin_family = AF_INET;
  transferAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  transferAddr.sin_port = htons(transferPort);
  bind(transferFd, (struct sockaddr*) &transferAddr, sizeof(transferAddr));
  listen(transferFd, 1024);

  printf("Init over\n");
  printf("CLIENT>");
  fflush(stdout);


  // Input area
  FD_ZERO(&master);
  FD_SET(0, &master);
  FD_SET(transferFd, &master);
  for(;;){
    read_fds = master;
    
    if(select(transferFd + 1, &read_fds, 0, 0, 0) == -1){
      return(1);
    }

    if(FD_ISSET(0, &read_fds)){
      getline(&message, &length, stdin);
      // The ls command
      if (strcmp(message, "ls\n") == 0){
        serverFd = socket(AF_INET, SOCK_STREAM, 0);
        connect(serverFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
        ls(serverFd, size, &serverAddr, masterList, &masterListPoint);
        close(serverFd);
      }
      // The get command
      else if(strcmp(message, "get\n") == 0){
        get(masterList, masterListPoint);
      }
      else{
        printf("Command not recognized\n");
      }
      // Making it look like a terminal
      printf("CLIENT>");
      fflush(stdout);
    }
    else if(FD_ISSET(transferFd, &read_fds)){
      struct sockaddr_in requestAddr;
      int requestSize;
      int connectionFd;
      requestSize = sizeof(requestAddr);
      printf("Getting a message\n");
      connectionFd = accept(transferFd, (struct sockaddr*) &requestAddr,
                            &requestSize);
      recvfrom(connectionFd, message, 1024, 0,
               (struct sockaddr *) &requestAddr, &requestSize);
      printf("%s\n", message);
      printf("End message\n");
    }
  }
}

