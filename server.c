#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#define MAX_NAME_SIZE 64
#define MAX_CLIENTS 10

struct clientEntry{
  char name[MAX_NAME_SIZE];
  struct sockaddr_in clientAddr;
};


int main(int argc, char* argv[]){
  int listenFd;
  int clientFd;
  int size;
  int length;
  int clientNamePointer = 0;
  struct clientEntry clientList[MAX_CLIENTS];
  struct clientEntry entry;
  struct sockaddr_in serverAddr;
  struct sockaddr_in clientAddr;
  socklen_t clientLen;
  char mesg[1024] = "TEST\n";

  listenFd = socket(AF_INET, SOCK_STREAM, 0);

  bzero(&serverAddr, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serverAddr.sin_port = htons(9000);
  
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

      break;
    }

    sendto(clientFd, mesg, size, 0, (struct sockaddr *) &clientAddr, length);
    
  }
}
