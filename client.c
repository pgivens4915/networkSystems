#include <stdio.h>
#include <sys/select.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define MAX_NAME_SIZE 64

void registerName(int serverFd, struct sockaddr_in* serverAddr, int size,
                  char* name){
  // 1 is the packet number that lets the server know
  // we are registering a name
  char packet[MAX_NAME_SIZE + 1] = "1";
  // Appending the strings
  strcat(packet, name);
  // Sending the string!
  sendto(serverFd, packet, MAX_NAME_SIZE + 1, 0,
        (struct sockaddr*) serverAddr, size);
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
  struct sockaddr_in serverAddr;
  struct sockaddr_in clientAddr;
  fd_set master;
  fd_set read_fds;
  size_t length;

  // Copying name from command line
  strcpy(name, argv[1]);
  // Getting the port number
  portNumber = atoi(argv[2]);

  // DEBUG message
  sprintf(message, "SENDING\n");
  // End debug messge
  
  FD_ZERO(&master);
  FD_ZERO(&master);

  FD_SET(0, &master);
  
  // Connecting to the server
  serverFd = socket(AF_INET, SOCK_STREAM, 0);
  bzero(&serverAddr, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serverAddr.sin_port = htons(portNumber);

  connect(serverFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

  size = sizeof(serverAddr);
  // Registering the clients name
  registerName(serverFd, &serverAddr, size, name);

  recvfrom(serverFd, message, 1024, 0, (struct sockaddr*) &serverAddr, &size);



  for(;;){
    read_fds = master;
    
    if(select(plusOne, &read_fds, 0, 0, 0) == -1){
      return(1);
    }

    if(FD_ISSET(0, &read_fds)){
      getline(&message, &length, stdin);
    }
  }
}

