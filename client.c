#include <stdio.h>
#include <sys/select.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]){
  char* message;
  int plusOne = 1;
  int listenFd;
  int serverFd;
  int size;
  struct sockaddr_in serverAddr;
  struct sockaddr_in clientAddr;
  fd_set master;
  fd_set read_fds;
  size_t length;
  
  FD_ZERO(&master);
  FD_ZERO(&master);

  FD_SET(0, &master);
  
  // Connecting to the server
  bzero(&serverAddr, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serverAddr.sin_port = htons(9000);

  bind(serverFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

  size = sizeof(serverAddr);
  sendto(serverFd, message, 1024, 0, (struct sockaddr*) &serverAddr, size);

  for(;;){
    read_fds = master;
    
    if(select(plusOne, &read_fds, 0, 0, 0) == -1){
      printf("Select Error\n");
      return(1);
    }

    if(FD_ISSET(0, &read_fds)){
      getline(&message, &length, stdin);
      printf(":%s:\n", message);
    }
  }
}
