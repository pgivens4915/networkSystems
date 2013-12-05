#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>


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
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serverAddr.sin_port = htons(9000);
  
  bind(listenFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

  printf("Before listen\n");
  listen(listenFd, 1024);

  for(;;){
    clientLen = sizeof(clientAddr);
    printf("Before accept\n");
    clientFd = accept(listenFd, (struct sockaddr *) &clientAddr, &clientLen);
    // Receiving bits
    length = sizeof(clientAddr);
    printf("Before recvfrom\n");
    size = recvfrom(clientFd, mesg, 1024, 0, (struct sockaddr *) &clientAddr,
                    &length);
    printf("%s", mesg);

    mesg[0] = '9';
    sendto(clientFd, mesg, size, 0, (struct sockaddr *) &clientAddr, length);
    printf("Transfer done");
    
  }
}