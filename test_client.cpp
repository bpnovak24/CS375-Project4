#include "ARQ.h"

int main(int argc, char **argv){
  Sender sender;//initialize sender
  int status = sender.setup_connection(argv[1]);//make sure input was correct
  if (status == 0){
    printf("Unable to successfully connect to host\n");
    exit(1);
  }
  sender.conversation();//talk to the receiver
  return 0;

}
