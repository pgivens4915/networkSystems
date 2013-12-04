#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>

int main(int argc, char* argv[]){
  int listenFd;
  int clientFd;
  int size;
  int length;
  struct sockaddr_in serverAddr;
  struct sockaddr_in clientAddr;
  socklen_t clientLen;
  char mesg[1024] = "TEST\n";

  listenFd = socket(AF_INET, SOCK_STREAM, 0);

  bzero(&serverAddr, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
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

    mesg[0] = '9';
    sendto(clientFd, mesg, size, 0, (struct sockaddr *) &clientAddr, length);
    printf("Transfer done");
    
  }
}
