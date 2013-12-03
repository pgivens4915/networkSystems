#include <stdio.h>
#include <sys/select.h>
#include <stdlib.h>


int main(int argc, char* argv[]){
  char* message = NULL;
  message = (char*)malloc(2034);
  int plusOne = 1;
  fd_set master;
  fd_set read_fds;
  size_t length;
  
  FD_ZERO(&master);
  FD_ZERO(&master);

  FD_SET(0, &master);
  
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
