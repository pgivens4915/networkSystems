#include <stdio.h>
#include <sys/sendfile.h>
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
      stat(ent->d_name, &st);
      fileSize = (long long) st.st_size;
      // Copying the file name into the table
      strcpy(fileTable[fileTablePointer].name, ent->d_name);
      // copying the hostname
      strcpy(fileTable[fileTablePointer].host, name);
      fileTable[fileTablePointer].size = fileSize;
      fileTable[fileTablePointer].port = transferPort;
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
  char mesg[50000] = "echo\n";
  int fileSock;
  int size;
  int length;
  FILE* fp;
  struct sockaddr_in fileAddr;
  printf("Enter filename :\n");
  scanf("%s", fileName);
  fp = fopen(fileName, "w");
  fileAddr = resolveAddress(masterList, masterListPoint, fileName);
  // If file not found
  if(fileAddr.sin_port == 0){
    return;
  }
  // Attempt to connect to the client with the file
  fileSock = socket(AF_INET, SOCK_STREAM, 0);
  connect(fileSock, (struct sockaddr*) &fileAddr, sizeof(fileAddr));
  // +1 for the charstop?
  sendto(fileSock, fileName, strlen(fileName) + 1, 0,
         (struct sockaddr*) &fileAddr, sizeof(fileAddr));
  recv(fileSock, &length, sizeof(int), 0);
  while ((size = recv(fileSock, mesg, 50000, 0)) > 0){
    printf("Send %i bytes out of %i\n", size, length);
    fprintf(fp, "%s", mesg);
    if (size == length){
      break;
    }
  }
  fflush(fp);
  fclose(fp);
  printf("Transfer Done\n");

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
  socklen_t channellen = sizeof(struct sockaddr_in);

  // Copying name from command line
  strcpy(name, argv[1]);
  // Getting the port number
  portNumber = atoi(argv[2]);

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

  // Opening a port for file transfer
  transferFd = socket(AF_INET, SOCK_STREAM, 0);
  bzero(&transferAddr, sizeof(transferAddr));
  transferAddr.sin_family = AF_INET;
  transferAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  bind(transferFd, (struct sockaddr*) &transferAddr, sizeof(transferAddr));
  getsockname(transferFd, (struct sockaddr*) &transferAddr, &channellen);
  transferPort = ntohs(transferAddr.sin_port);
  printf("Port %i\n", transferPort);
  listen(transferFd, 1024);


  // Registering the clients name
  registerName(serverFd, &serverAddr, size, name, transferPort);
  close(serverFd);

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
      else if(strcmp(message, "exit\n") == 0){
        // 3 means exit
        char messageType[1 + MAX_NAME_SIZE] = "3";
        serverFd = socket(AF_INET, SOCK_STREAM, 0);
        connect(serverFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
        strcat(messageType, name); 
        send(serverFd, messageType, sizeof(char) * MAX_NAME_SIZE + 1, 0);
        close(serverFd);
        return(0);
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
      int size;
      FILE* fp;
      struct stat stat;
      requestSize = sizeof(requestAddr);
      connectionFd = accept(transferFd, (struct sockaddr*) &requestAddr,
                            &requestSize);
      recvfrom(connectionFd, message, 1024, 0,
               (struct sockaddr *) &requestAddr, &requestSize);
      fp = fopen(message, "r");
      // Send dat file!
      fstat(fileno(fp), &stat);
      size = stat.st_size;
      send(connectionFd, &size, sizeof(int), 0);
      sendfile(connectionFd, fileno(fp), 0, stat.st_size);
    }
  }
}

